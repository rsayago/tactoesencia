<?php

/**
 * @class ICalendar parser for availability calendars.
 */
class AvailabilityCalendarICalFeedsParser extends FeedsParser {

  /**
   * Implements FeedsParser::parse().
   *
   * @param \FeedsSource $source
   * @param \FeedsFetcherResult $fetcher_result
   *
   * @return \FeedsParserResult
   *
   * @throws \Exception
   */
  public function parse(FeedsSource $source, FeedsFetcherResult $fetcher_result) {
    $state = $source->state(FEEDS_PARSE);

    // Read the iCal feed into memory.
    $ical_feed_contents = $fetcher_result->getRaw();

    $vevents = $this->parseVcalendar($ical_feed_contents);

    // Report progress.
    // We need to add 1 to the index of the last parsed component so that
    // the subsequent batch starts on the first unparsed component.
    $state->progress($state->total, $state->pointer);

    // All the vevents are to be handled as just 1 item with 1 field.
    return new FeedsParserResult(array(array('vcalendar' => $vevents)));
  }

  /**
   * Parses a VCALENDAR feed into VEVENTS.
   *
   * @param string $feed
   *
   * @return \stdClass[]
   *   A list of VEVENT objects.
   *
   * @throws \Exception
   */
  protected function parseVcalendar($feed) {
    $vevents = array();
    $lines = explode("\n", str_replace(array("\r\n", "\n\r", "\r"), "\n", $feed));
    $i = 0;
    $c = count($lines);
    while ($i < $c) {
      if (strtoupper(trim($lines[$i++])) === 'BEGIN:VEVENT') {
        $vevent = $this->parseVevent($lines, $i);
        if (is_object($vevent)) {
          $vevents[] = $vevent;
        }
      }
    }
    return $vevents;
  }

  /**
   * Parses a VEVENT in a VCALENDAR feed.
   *
   * @param array $lines
   * @param int $i
   *
   * @return bool|\stdClass
   *
   * @throws \Exception
   */
  protected function parseVevent(array $lines, &$i) {
    $vevent = new stdClass();
    $c = count($lines);
    while ($i < $c) {
      $parts = explode(':', $lines[$i++]);
      if (count($parts) !== 2) {
        return FALSE;
      }
      $property = strtoupper(trim($parts[0]));
      $value = $parts[1];

      $propertyParts = explode(';', $property);
      if (count($propertyParts) > 1) {
        $property = $propertyParts[0];
        $propertyType = strtoupper(trim($propertyParts[1]));
      }
      else {
        $propertyType = '';
      }
      switch ($property) {
        case 'UID':
          if ($propertyType !== '') {
            return FALSE;
          }
          $vevent->uid = $value;
          break;
        case 'SUMMARY':
          if ($propertyType !== '') {
            return FALSE;
          }
          $vevent->summary = $value;
          break;
        case 'DTSTART':
          if ($propertyType !== 'VALUE=DATE') {
            return FALSE;
          }
          $vevent->start = DateTime::createFromFormat('Ymd', $value)
            ->setTime(0, 0, 0, 0);
          break;
        case 'DTEND':
          if ($propertyType !== 'VALUE=DATE') {
            return FALSE;
          }
          // A DTEND indicates the departure day, we store a not-available block
          // from the arrival date just to the last full day, ie. the day before
          // the departure.
          $vevent->end = DateTime::createFromFormat('Ymd', $value)
            ->setTime(0, 0, 0, 0)
            ->sub(new DateInterval('P1D'));
          break;
        case 'END':
          if ($propertyType !== '') {
            return FALSE;
          }
          if (strtoupper(trim($value)) !== 'VEVENT') {
            return FALSE;
          }
          return !empty($vevent->start) && !empty($vevent->end) ? $vevent : FALSE;
          break;
        default:
          // We ignore other properties.
          break;
      }
    }
    return FALSE;
  }

}
