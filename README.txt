This is a drupal module to implement a generic workflow around Fedora objects.

It uses an inline xml datastream to record the state of the workflow and notes associated with the workflow in workflow managed objects.
The module currently assumes that the '' object acts as a parent for collection content models and not directly as a content model.  This can be changed via modifying 'collection_query.txt'.

Machine level workflow will be handled via fedora microservices.
Human level workflow will be handled through drupal.

Acceptable state values: created, in_processing(for machine workflow), submitted, approved, published, rejected