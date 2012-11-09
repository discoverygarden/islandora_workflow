<?php

/**
 * @file
 *   Implement the module theme functions.
 */

 /**
  * Theme preprocess function.
  *
  * This function handles making variables available to the theme template for
  * the islandora_workflow_collections page
  *
  * @param array $vars
  *   the list of variables (including the form) to send to the template
  *
  * @return array
  *   $vars the modified list of variables to send to the template
  */
function islandora_workflow_preprocess_workflow_collections(&$vars) {
  module_load_include('inc', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.collection');
  module_load_include('inc', 'islandora_workflow', 'fedora_object_functions');

  $collections = islandora_workflow_get_collections();

  $vars['collections'] = $collections;
  $vars['list']['submit'] = drupal_render($vars['list']['submit']);
  $active_collection_pid = $_SESSION['workflow_permissions_page']['active_collection'];
  $vars['list']['active_collection_pid'] = $active_collection_pid;
  $vars['list']['active_collection_label'] = islandora_workflow_get_object_label($active_collection_pid);
  // Tender each form element.
  foreach ($collections as $collection_pid => $collection_information) {
    $vars['list']['collection_checkboxes'][$collection_pid] = drupal_render($vars['list'][$collection_pid]);
  }
}

 /**
  * Theme preprocess function.
  *
  * This function handles making variables available to the theme template for
  * the islandora_workflow_collection_permissions page (user tab)
  *
  * @param array $vars
  *   the list of variables (including the form) to send to the template
  *
  * @return array
  *   $vars the modified list of variables to send to the template
  */
function islandora_workflow_preprocess_permissions_user_table(&$vars) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');
  module_load_include('inc', 'islandora_workflow', 'fedora_object_functions');

  $vars['users'] = islandora_workflow_get_users_with_module_permissions();
  $vars['list']['collection_selector'] = drupal_render($vars['list']['collection_selector']);
  $vars['list']['collection_submit'] = drupal_render($vars['list']['collection_submit']);
  $active_collection_pid = $_SESSION['workflow_permissions_page']['active_collection'];
  $vars['list']['active_collection_pid'] = $active_collection_pid;
  $vars['list']['active_collection_label'] = islandora_workflow_get_object_label($active_collection_pid);


  // Render each form element.
  foreach ($vars['users'] as $user_id => $user_name) {
    $vars['list'][$user_id][$active_collection_pid] = drupal_render($vars['list'][$user_id][$active_collection_pid]);
  }
}

/**
 * Theme preprocess function.
 *
 * This function handles making variables available to the theme template for
 * the islandora_workflow_collection_permissions page (role tab)
 *
 * @param array $vars
 *   the list of variables (including the form) to send to the template
 *
 * @return array
 *   $vars the modified list of variables to send to the template
 */
function islandora_workflow_preprocess_permissions_role_table(&$vars) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');
  module_load_include('inc', 'islandora_workflow', 'fedora_object_functions');

  $vars['roles'] = islandora_workflow_get_roles_with_module_permissions();
  $vars['list']['collection_selector'] = drupal_render($vars['list']['collection_selector']);
  $vars['list']['collection_submit'] = drupal_render($vars['list']['collection_submit']);

  $active_collection_pid = $_SESSION['workflow_permissions_page']['active_collection'];
  $vars['list']['active_collection_pid'] = $active_collection_pid;
  $vars['list']['active_collection_label'] = islandora_workflow_get_object_label($active_collection_pid);

  // Render each form element.
  foreach ($vars['roles'] as $role_id => $role_name) {
    $vars['list'][$role_id][$active_collection_pid] = drupal_render($vars['list'][$role_id][$active_collection_pid]);
  }
}

/**
 * Theme function for the workflow form.
 *
 * @param array $form
 *   the form being themed
 *
 * @return string
 *   the html to render
 */
function theme_workflow_table($form = NULL) {
  // Little hacky, but we get the headers from the form.
  $headers = $form['header']['#value'];

  $rows = array();

  foreach ($form as $key => $value) {
    // Array check prevents notice.
    if (is_array($value)) {
      if (isset($value['table_info']['#value'])) {
        $info = $value['table_info']['#value'];
        $rows[] = array(
          drupal_render($value['Selecter']),
          l($info['label'] ? $info['label'] : $key, "fedora/repository/$key"),
          $info['collection_label'] ? $info['collection_label'] : $info['collection_object'],
          $info['state'],
          format_date(strtotime($info['created_date']), $type = 'small'),
          format_date(strtotime($info['islandora_workflow_modified']), $type = 'small'),
          l($info['subject'] ? $info['subject'] : t('Create Note'), 'islandora_workflow_edit_note', array('query' => 'object=' . $key)),
          drupal_render($value['Assign']),
        );
      }
    }
  }

  return theme_table($headers, $rows);
}
