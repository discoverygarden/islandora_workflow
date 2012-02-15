<?php
/**
 * @file
 *   This template displays the form for collection workflow state.
 */
  foreach ($collections as $collection_pid => $collection_label) {
    print($vars['list']['collection_checkboxes'][$collection_pid]);
  }
  print($vars['list']['submit']);
?>