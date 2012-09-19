<?php
/**
 * @file
 *   This template displays the form for collection workflow state.
 */

  foreach ($collections as $collection_pid => $collection_information) :
    print($list['collection_checkboxes'][$collection_pid]);
  endforeach;
  print($list['submit']);
?>
