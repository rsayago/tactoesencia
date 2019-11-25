<?php
/**
 * Availability Calendar ICalendar code.
 */

module_load_include('inc', 'availability_calendar');

/**
 * Implements hook_process_HOOK for theme availability_calendar_ical.
 *
 * On entry, $variables contains:
 * - entity_type
 * - entity
 * - settings (field settings)
 * - langcode
 * - cid
 * - name
 *
 * On exit it should also contain:
 * - from
 * - to
 * - events

 * @param array $variables
 *
 * @throws \Exception
 */
function availability_calendar_process_availability_calendar_ical(&$variables) {
  if (empty($variables['events'])) {
    if (empty($variables['from'])) {
      $variables['from'] = (new DateTime())->setTime(0, 0, 0, 0);
    }
    if (empty($variables['to'])) {
      $months_to_render = !empty($variables['settings']['show_number_of_months']) ? $variables['settings']['show_number_of_months'] : 12;
      $to = clone $variables['from'];
      $to->add(new DateInterval("P{$months_to_render}M"));
      $to->sub(new DateInterval('P1D'));
      $variables['to'] = $to;
    }
    // @todo: ensure $variables['settings']['default_state']
    $variables['events'] = availability_calendar_get_unavailable_periods((int) $variables['cid'], $variables['from'], $variables['to'], $variables['settings']['default_state']);
  }
}

/**
 * Themes the availability calendar field as an ICalendar feed.
 *
 * It does so by merely passing the variables to theme
 * availability_calendar_vcalendar.
 *
 * $variables contains:
 * - cid
 * - name
 * - from
 * - to
 * - events
 *
 * @param array $variables
 *
 * @return string
 * @throws \Exception
 */
function theme_availability_calendar_ical($variables) {
  $output = theme('availability_calendar_ical_vcalendar', $variables);
  return $output;
}

/**
 * Implements hook_process_HOOK for theme availability_calendar_ical_vcalendar.
 *
 * On entry, $variables contains:
 * - entity_type
 * - entity
 * - settings (field settings)
 * - langcode?
 * - cid
 * - name
 * - events
 *
 * This process hook adds other variables the theme template expects:
 * - prodId
 * - title
 * - language
 * - method
 *
 * @param array $variables
 */
function availability_calendar_process_availability_calendar_ical_vcalendar(&$variables) {
  // Ensure good defaults for missing variables.
  if (empty($variables['prodId'])) {
    $variables['prodId'] = variable_get('site_name', 'Drupal');
    $slogan = variable_get('site_slogan', '');
    if (!empty($slogan)) {
      $variables['prodId'] .= ' - ' . $slogan;
    }
  }

  if (empty($variables['title'])) {
    if (!empty($variables['entity_type']) && !empty($variables['entity'])) {
      $variables['title'] = entity_label($variables['entity_type'],
        $variables['entity']);
    }
  }

  if (empty($variables['langcode']) || $variables['langcode'] === LANGUAGE_NONE) {
    global $language;
    $variables['langcode'] = $language->language;
  }

  if (empty($variables['method'])) {
    $variables['method'] = 'PUBLISH';
  }

  // Live preview: escape output. : @todo: check that & etc remained.
  $variables['title'] = filter_xss_admin($variables['title']);
  $variables['prodId'] = filter_xss_admin($variables['prodId']);
  $variables['langcode'] = check_plain($variables['langcode']);
  $variables['method'] = check_plain($variables['method']);
}

/**
 * Implements hook_process_HOOK for theme availability_calendar_ical_vevent.
 *
 * - uid  A unique identifier for the event.
 * - start  The start date of the event (yyyymmdd).
 * - end  The end date of the event (yyyymmdd).
 * - summary  A summary of the event (free format).
 *
 * @param array $variables
 */
function availability_calendar_process_availability_calendar_ical_vevent(&$variables) {
  if (empty($variables['uid'])) {
    $variables['uid'] = $variables['start'] . '-' . $variables['end'] . '.' . $variables['title'] . '@' . (!empty($_SERVER['HTTP_HOST']) ? $_SERVER['HTTP_HOST'] : $_SERVER['SERVER_NAME']);
  }
  if (empty($variables['summary'])) {
    $variables['summary'] = t('Unavailable', array(), array('langcode' => $variables['langcode']));
  }
}
