<?php

/**
 *
 * @param array $options
 *   A set of options.
 * @return array
 */
function _islandora_workflow_base_rule($options) {
  $base_rule = array(
    '#type' => 'rule',
    '#set' => $options['set'],
    '#label' => $options['label'],
    '#active' => 1,
    '#weight' => '0',
    '#categories' => array('workflow'),
    '#status' => 'default',
    '#conditions' => array(),
    '#actions' => array(
      0 => array(
        '#weight' => 0,
        '#info' => array(
          'label' => $options['action_label'],
          'module' => 'Islandora Workflow',
          'arguments' => array(
            'object' => array(
              'type' => 'fedora_object',
              'label' => $options['object_label'],
            ),
          ),
          'eval input' => array(
            0 => 'message',
          ),
        ),
        '#name' => 'islandora_workflow_rules_email_someone',
        '#settings' => array(
          'message' => $options['settings']['message'],
          // An array with key/value pairs of the permission name.
          'roles' => $options['settings']['roles'],
          'notify_creator' => $options['settings']['notify_creator'],
          '#argument map' => array(
            'object' => 'object',
          ),
          '#eval input' => array(
            'token_rules_input_evaluator' => array(
              'message' => array(
                0 => ':global',
              ),
            ),
          ),
        ),
        '#type' => 'action',
      ),
    ),
    '#version' => 6003,
  );
  return $base_rule;
}

/**
 * Implements hook_rules_defaults().
 */
function islandora_workflow_rules_defaults() {

  $approved_options = array(
    'set' => 'event_islandora_workflow_rules_approve_object',
    'label' => 'A Fedora object is approved',
    'action_label' => 'Send email notifications',
    'object_label' => 'The object that has been approved',
    'settings' => array(
      'message' => '',
      'roles' => array('islandora_workflow_Editor'),
      'notify_creator' => 1,
    ),
  );
  $rejected_options = array(
    'set' => 'event_islandora_workflow_rules_reject_object',
    'label' => 'A Fedora object is rejected',
    'action_label' => 'Send email notifications',
    'object_label' => 'The object that has been rejected',
    'settings' => array(
      'message' => '',
      'roles' => array('islandora_workflow_Editor'),
      'notify_creator' => 1,
    ),
  );
  $submitted_options = array(
    'set' => 'event_islandora_workflow_rules_submit_object',
    'label' => 'A Fedora object is submitted',
    'action_label' => 'Send email notifications',
    'object_label' => 'The object that has been submitted',
    'settings' => array(
      'message' => '',
      'roles' => array('islandora_workflow_Editor'),
      'notify_creator' => 0,
    ),
  );
  $assigned_options = array(
    'set' => 'event_islandora_workflow_rules_assign_object',
    'label' => 'A Fedora object is assigned',
    'action_label' => 'Send email notifications',
    'object_label' => 'The object that has been assigned',
    'settings' => array(
      'message' => '',
      'roles' => array('islandora_workflow_Editor'),
      'notify_creator' => 1,
    ),
  );

  $config = array(
    'rules' => array(
      'iw_rules_email_item_approved' => _islandora_workflow_base_rule($approved_options),
      'iw_rules_email_item_rejected' => _islandora_workflow_base_rule($rejected_options),
      'iw_rules_email_item_submitted' => _islandora_workflow_base_rule($submitted_options),
      'iw_rules_email_item_assigned' => _islandora_workflow_base_rule($assigned_options),
    ),
    'rule sets' => array(
      // no rule sets
    ),
  );
  return $config;
}
