<?php
/**
 * @file
 *   A default set of rules for the workflow module.
 */

/**
 * A helper function for generating a rule.
 *
 * @param array $options
 *   A set of options.
 *
 * @return array
 *   A rule datastructure.
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
            'message', 'subject',
          ),
        ),
        '#name' => 'islandora_workflow_rules_email_someone',
        '#settings' => array(
          'subject' => $options['settings']['subject'],
          'message' => $options['settings']['message'],
          'html_message' => $options['settings']['html_message'],
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
                'object', ':global',
              ),
            ),
          ),
        ),
        '#type' => 'action',
      ),
      1 => array(
        '#weight' => 0,
        '#info' => array(
          'label' => t('Show a configurable message on the site'),
          'module' => 'System',
          'eval input' => array(
            'message',
          ),
        ),
        '#name' => 'rules_action_drupal_message',
        '#settings' => array(
          'message' => $options['show_message'],
          'error' => 0,
          '#eval input' => array(
            'token_rules_input_evaluator' => array(
              'message' => array(
                'object', ':global',
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
  $options = array(
    'approved' => array(
      'set' => 'event_islandora_workflow_rules_approve_object',
      'label' => t('A Fedora object is approved'),
      'action_label' => t('Send email notifications'),
      'object_label' => t('The object that has been approved'),
      'settings' => array(
        'subject' => 'An object has been approved',
        'message' => 'The object <em>[object:fedora-object-title]</em> ([object:fedora-object-pid]) has been approved and is awaiting publication.',
        'roles' => array('islandora_workflow_Editor'),
        'notify_creator' => 1,
        'notify_assignee' => 0,
      ),
      'show_message' => 'The object <em>[object:fedora-object-title]</em> ([object:fedora-object-pid]) has been approved.',
    ),
    'rejected' => array(
      'set' => 'event_islandora_workflow_rules_reject_object',
      'label' => t('A Fedora object is rejected'),
      'action_label' => t('Send email notifications'),
      'object_label' => t('The object that has been rejected'),
      'settings' => array(
        'subject' => 'An object has been rejected',
        'message' => 'The object <em>[object:fedora-object-title]</em> ([object:fedora-object-pid]) has been rejected.',
        'roles' => array('islandora_workflow_Editor'),
        'notify_creator' => 1,
        'notify_assignee' => 0,
      ),
      'show_message' => 'The object <em>[object:fedora-object-title]</em> ([object:fedora-object-pid]) has been rejected.',
    ),
    'submitted' => array(
      'set' => 'event_islandora_workflow_rules_submit_object',
      'label' => t('A Fedora object is submitted'),
      'action_label' => t('Send email notifications'),
      'object_label' => t('The object that has been submitted'),
      'settings' => array(
        'subject' => 'An object has been submitted',
        'message' => 'The object <em>[object:fedora-object-title]</em> ([object:fedora-object-pid]) has been submitted for approval.',
        'roles' => array('islandora_workflow_Editor'),
        'notify_creator' => 0,
        'notify_assignee' => 0,
      ),
      'show_message' => 'The object <em>[object:fedora-object-title]</em> ([object:fedora-object-pid]) has been submitted.',
    ),
    'assigned' => array(
      'set' => 'event_islandora_workflow_rules_assign_object',
      'label' => t('A Fedora object is assigned'),
      'action_label' => t('Send email notifications'),
      'object_label' => t('The object that has been assigned'),
      'settings' => array(
        'subject' => 'An object has been assigned',
        'message' => 'The object <em>[object:fedora-object-title]</em> ([object:fedora-object-pid]) has been assigned.',
        'roles' => array(),
        'notify_creator' => 0,
        'notify_assignee' => 1,
      ),
      'show_message' => 'The object <em>[object:fedora-object-title]</em> ([object:fedora-object-pid]) has been assigned.',
    ),
  );

  $message_footer = ' Please visit ' . l(t('The Islandora Workflow pages'), 'islandora_workflow', array('absolute' => TRUE)) . ' to take action.';

  $config = array(
    'rules' => array(),
    'rule sets' => array(
      // No rule sets.
    ),
  );

  foreach ($options as $type => $info) {
    $key = 'iw_rules_email_item_' . $type;
    $info['settings']['message'] .= $message_footer;
    $info['settings']['html_message'] = TRUE;
    $config['rules'][$key] = _islandora_workflow_base_rule($info);
  }

  return $config;
}
