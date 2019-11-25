<?php

/**
 * @class ICalendar processor for availability calendars.
 */
class AvailabilityCalendarICalFeedsProcessor extends FeedsProcessor {

  public function configDefaults() {
    module_load_include('inc', 'availability_calendar');
    $result = parent::configDefaults();

    // Overwrite some defaults.
    $result['insert_new'] = FEEDS_SKIP_NEW;
    $result['update_existing'] = FEEDS_UPDATE_EXISTING;
    $result['update_non_existent'] = FEEDS_SKIP_NON_EXISTENT;

    // Add our own config defaults.
    $unavailableStates = availability_calendar_get_states(false);
    reset($unavailableStates);
    $result['availability_calendar_state'] = key($unavailableStates);
    $result['entity_type'] = '';
    $result['bundle'] = '';
    $result['entity_id'] = 0;

    return $result;
  }

  public function configForm(&$form_state) {
    $form = array();

    $form['skip_hash_check'] = array(
      '#type' => 'checkbox',
      '#title' => t('Skip hash check'),
      '#description' => t('Force update of items even if item source data did not change.'),
      '#default_value' => $this->config['skip_hash_check'],
    );

    $form['availability_calendar_state'] = array(
      '#type' => 'select',
      '#title' => t('Availability state to use for the (unavailable) periods in the iCal feed.'),
      '#options' => availability_calendar_get_states(false, 'label'),
      '#default_value' => $this->config['availability_calendar_state'],
      '#required' => TRUE,
    );

    $entity_infos = entity_get_info();
    $options = array(0 => t('Select one'));
    foreach ($entity_infos as $type => $entity_info) {
      $options[$type] = $entity_info['label'];
    }
    $form['entity_type'] = array(
      '#type' => 'select',
      '#title' => t('The entity type of the entity this iCal feed should be added to.'),
      '#options' => $options,
      '#default_value' => $this->config['entity_type'],
      '#required' => TRUE,
    );
    $form['entity_id'] = array(
      '#type' => 'textfield',
      '#title' => t('The entity id of the entity this iCal feed should be added to.'),
      '#default_value' => $this->config['entity_id'],
      '#required' => TRUE,
    );

    return $form;
  }

  public function configFormValidate(&$values) {
    parent::configFormValidate($values);

    $entity = $this->entity_load($values['entity_id'], $values['entity_type']);
    if (!is_object($entity)) {
      form_set_error('entity_id', t('The entity does not exist.'));
    }
  }

  public function entityType() {
    return $this->config['entity_type'];
  }

  /**
   * Returns the bundle for the target entity.
   *
   * @return string
   *
   * @throws \EntityMalformedException
   */
  public function bundle() {
    $bundle = '';
    $entity = $this->entity_load($this->config['entity_id']);
    if (is_object($entity)) {
      $eids = entity_extract_ids($this->entityType(), $entity);
      $bundle = $eids[2];
    }
    return $bundle;
  }

  /**
   * Declare possible mapping targets that this processor exposes.
   *
   * @ingroup mappingapi
   *
   * @return array
   *   An array of mapping targets. Keys are paths to targets
   *   separated by ->, values are TRUE if target can be unique,
   *   FALSE otherwise.
   *
   * @throws \EntityMalformedException
   */
  public function getMappingTargets() {
    $targets = array();

    foreach (field_info_instances($this->entityType(), $this->bundle()) as $name => $instance) {
      $info = field_info_field($name);
      if ($info['type'] === 'availability_calendar') {
        $targets[$name] = array(
          'name' => $instance['label'],
          'description' => t('The target availability calendar to add the events to.'),
        );
      }
    }

    return $targets;
  }

  /**
   * Execute mapping on an item.
   *
   * @ingroup mappingapi
   *
   * @param \FeedsSource $source
   * @param \FeedsParserResult $result
   * @param object|null $target_item
   *
   * @return null
   * @throws \EntityMalformedException
   * @throws \Exception
   */
  protected function map(FeedsSource $source, FeedsParserResult $result, $target_item = NULL) {

    // Static cache $targets as getMappingTargets() may be an expensive method.
    static $sources;
    if (!isset($sources[$this->id])) {
      $sources[$this->id] = feeds_importer($this->id)->parser->getMappingSources();
    }
    static $targets;
    if (!isset($targets[$this->id])) {
      $targets[$this->id] = $this->getMappingTargets();
    }
    /** @var \FeedsParser $parser */
    $parser = feeds_importer($this->id)->parser;

    foreach ($this->config['mappings'] as $mapping) {
      $value = $parser->getSourceElement($source, $result, $mapping['source']);

      // Map the source element's value to the target.
      $this->setTargetElement($source, $target_item, $mapping['target'], $value);
    }

    return $target_item;
  }

  /**
   * Set a concrete target element. Invoked from FeedsProcessor::map().
   *
   * @ingroup mappingapi
   *
   * @param \FeedsSource $source
   * @param object $target_item
   * @param string $target_element
   * @param mixed $value
   *
   * @throws \Exception
   */
  public function setTargetElement(FeedsSource $source, $target_item, $target_element, $value) {
    $calendarField = $target_item->$target_element;
    $calendar = reset(reset($calendarField));
    $field_info = field_info_field($target_element);
    $defaultState = $field_info['settings']['default_state'];
    $newState = $this->config['availability_calendar_state'];

    $cid = $calendar['cid'];
    $updated = 0;
    $skipped = 0;
    foreach ($value as $vevent) {
      if ($this->update1Period($cid, $newState, $vevent->start, $vevent->end, $defaultState)) {
        $updated++;
      } else {
        $skipped++;
      }
    }

    $eids = entity_extract_ids($this->entityType(), $target_item);
    $entity_id = $eids[0];
    $message = t('Imported iCal feed for @entity_type %entity_id (cid = %cid): updated %updated periods; skipped %skipped periods',
      array(
        '@entity_type' => $this->entityType(),
        '%entity_id' => $entity_id,
        '%cid' => $cid,
        '%updated' => $updated,
        '%skipped' => $skipped,
      ));
    $source->log('import', $message, array(), WATCHDOG_INFO);
  }

  /**
   * Updates 1 period on the availability calendar.
   *
   * @param int $cid
   * @param int $newState
   * @param DateTime $from
   * @param DateTime $to
   * @param int $defaultState
   *
   * @return bool
   *   True if some (or all) dates in the period were updated or false if all
   *   dates already have $newState as availability state.
   *
   * @throws \Exception
   */
  protected function update1Period($cid, $newState, $from, $to, $defaultState) {
    $availability = availability_calendar_get_availability($cid, $from, $to, $defaultState);
    $currentStates = array_unique($availability, SORT_NUMERIC);
    if (count($currentStates) > 1 || reset($currentStates) != $newState) {
      availability_calendar_update_availability($cid, $newState, $from, $to);
      return TRUE;
    }
    return FALSE;
  }

  /**
   * {@inheritDoc}
   */
  protected function existingEntityId(FeedsSource $source, FeedsParserResult $result) {
    return $this->config['entity_id'];
  }

  /**
   * {@inheritDoc}
   */
  protected function entityLoad(FeedsSource $source, $entity_id) {
    return $this->entity_load($entity_id);
  }

  /**
   * Load an existing entity.
   *
   * @param $entity_id
   *   The unique id of the entity that should be loaded.
   * @param string $entity_type
   *
   * @return object|null
   *   An existing entity object.
   */
  protected function entity_load($entity_id, $entity_type = '') {
    $entities = entity_load(!empty($entity_type) ? $entity_type : $this->entityType(), array($entity_id));
    return is_array($entities) ? reset($entities) : NULL;
  }

  /**
   * {@inheritDoc}
   */
  protected function newEntity(FeedsSource $source) {
    throw new RuntimeException(__METHOD__ . ' not implemented.');
  }

  /**
   * {@inheritDoc}
   */
  protected function entitySave($entity) {
    entity_save($this->entityType(), $entity);
  }

  /**
   * {@inheritDoc}
   */
  protected function entityDeleteMultiple($entity_ids) {
    throw new RuntimeException(__METHOD__ . ' not implemented.');
  }
}
