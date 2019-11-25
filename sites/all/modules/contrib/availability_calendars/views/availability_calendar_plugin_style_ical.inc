<?php

/**
 * @file
 * Views style plugin for the iCal feed.
 */

/**
 * Default style plugin to render an iCal feed.
 */
class availability_calendar_plugin_style_ical extends views_plugin_style {

  /**
   * Allow the style to do stuff before each row is rendered.
   *
   * @param array $result
   *   The full array of results from the query.
   */
  public function pre_render($result) {
    $view = $this->view;
    if (empty($view->live_preview)) {
      // We are in the feed: add content-type header and prevent the devel module
      // appending queries to the feed.
      drupal_add_http_header('Content-Type', 'application/calendar; charset=utf-8');

      // Do not let the devel module append queries to the ical feed.
      $GLOBALS['devel_shutdown'] = FALSE;
    }
  }

  /**
   * Renders the display in this style.
   */
  public function render() {
    $renderResult = parent::render_fields($this->view->result);

    $output = '';
    foreach ($renderResult as $row) {
      foreach ($row as $field) {
        $output .= $field;
      }
    }
    return $output;
  }
}
