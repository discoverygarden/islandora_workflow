<?php

function _islandora_workflow_rules_message($rule_label, $event, $message) {
  $rule = array(
    'rules' => array(
      'rules_1' => array(
        '#type' => 'rule',
        '#set' => $event,
        '#label' => $rule_label,
        '#active' => 1,
        '#weight' => '0',
        '#categories' => array(
          0 => 'workflow',
        ),
        '#status' => 'default',
        '#conditions' => array(),
        '#actions' => array(
          0 => array(
            '#weight' => 0,
            '#info' => array(
              'label' => 'Show a configurable message on the site',
              'module' => 'System',
              'eval input' => array(
                0 => 'message',
              ),
            ),
            '#name' => 'rules_action_drupal_message',
            '#settings' => array(
              'message' => $message,
              'error' => 0,
              '#eval input' => array(
                'token_rules_input_evaluator' => array(
                  'message' => array(
                    0 => 'object',
                    1 => ':global',
                  ),
                ),
              ),
            ),
            '#type' => 'action',
          ),
        ),
      ),
    ),
    '#version' => 6003,
  );
  return $rule;
}

/**
 *
 * @param array $options
 *   A set of options.
 *
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
          'notify_assignee' => $options['settings']['notify_assignee'],
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
      'message' => 'Object [object:fedora-object-pid] has been approved and is awaiting publication.',
      'roles' => array('islandora_workflow_Editor'),
      'notify_creator' => 1,
      'notify_assignee' => 0,
    ),
  );
  $rejected_options = array(
    'set' => 'event_islandora_workflow_rules_reject_object',
    'label' => 'A Fedora object is rejected',
    'action_label' => 'Send email notifications',
    'object_label' => 'The object that has been rejected',
    'settings' => array(
      'message' => 'Object [object:fedora-object-pid] has been rejected.',
      'roles' => array('islandora_workflow_Editor'),
      'notify_creator' => 1,
      'notify_assignee' => 0,
    ),
  );
  $submitted_options = array(
    'set' => 'event_islandora_workflow_rules_submit_object',
    'label' => 'A Fedora object is submitted',
    'action_label' => 'Send email notifications',
    'object_label' => 'The object that has been submitted',
    'settings' => array(
      'message' => 'Object [object:fedora-object-pid] has been submitted for approval.',
      'roles' => array('islandora_workflow_Editor'),
      'notify_creator' => 0,
      'notify_assignee' => 0,
    ),
  );
  $assigned_options = array(
    'set' => 'event_islandora_workflow_rules_assign_object',
    'label' => 'A Fedora object is assigned',
    'action_label' => 'Send email notifications',
    'object_label' => 'The object that has been assigned',
    'settings' => array(
      'message' => 'Object [object:fedora-object-pid] has been assigned to you.',
      'roles' => array(),
      'notify_creator' => 0,
      'notify_assignee' => 1,
    ),
  );

  $message_footer = ' Please visit ' . l(t('The Islandora Workflow pages'), 'islandora_workflow') . ' to take action.';
  $approved_options['settings']['message'] .= $message_footer;
  $rejected_options['settings']['message'] .= $message_footer;
  $submitted_options['settings']['message'] .= $message_footer;
  $assigned_options['settings']['message'] .= $message_footer;

  $config = array(
    'rules' => array(
      'iw_rules_email_item_approved' => _islandora_workflow_base_rule($approved_options),
      'iw_rules_email_item_rejected' => _islandora_workflow_base_rule($rejected_options),
      'iw_rules_email_item_submitted' => _islandora_workflow_base_rule($submitted_options),
      'iw_rules_email_item_assigned' => _islandora_workflow_base_rule($assigned_options),
      'iw_rules_message_item_approved' => _islandora_workflow_rules_message('Display a Drupal message when an object is approved', 'event_islandora_workflow_rules_approve_object', 'The object [object:fedora-object-pid] has been approved.'),
      'iw_rules_message_item_rejected' => _islandora_workflow_rules_message('Display a Drupal message when an object is rejected', 'event_islandora_workflow_rules_reject_object', 'The object [object:fedora-object-pid] has been rejected.'),
      'iw_rules_message_item_submitted' => _islandora_workflow_rules_message('Display a Drupal message when an object is submitted', 'event_islandora_workflow_rules_submit_object', 'The object [object:fedora-object-pid] has been submitted.'),
      'iw_rules_message_item_assigned' => _islandora_workflow_rules_message('Display a Drupal message when an object is assigned', 'event_islandora_workflow_rules_assign_object', 'The object [object:fedora-object-pid] has been assigned.'),
      'iw_rules_message_item_deleted' => _islandora_workflow_rules_message('Display a Drupal message when an object is deleted', 'event_islandora_workflow_rules_delete_object', 'The object [object:fedora-object-pid] has been deleted.'),
    ),
    'rule sets' => array(
      // No rule sets.
    ),
  );
  return $config;
}
