<?php
/**
 * @file
 *   This template displays the form for collection workflow state.
 */
  foreach ($collections as $collection_pid => $collection_label) {
    print($collection_checkboxes[$collection_pid]);
  }
  print($submit);
?>