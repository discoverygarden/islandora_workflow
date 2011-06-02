This is a drupal module to implement a generic workflow around Fedora objects.
It uses an inline xml datastream to record the state of the workflow and notes associated with the workflow.

Machine level workflow will be handled via fedora microservices.
Human level workflow will be handled through drupal.

Acceptable state values: created, in_processing, submitted, approved, published, rejected