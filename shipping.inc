<?php

/**
 * @file
 * Commerce Migrate Ubercart Shipping
 * Migration class to bring shipping costs to commerce order line item
 */

/**
 * @author David Kitchen
 *
 */
class CommerceMigrateUbercartShippingMigration extends Migration {
  public function __construct() {
    parent::__construct();
    $this->description = t('Import shipping line items from Ubercart.');
    $this->dependencies = array('CommerceMigrateUbercartOrder');

    // Create a map object for tracking the relationships between source rows.
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'line_item_id' => array(
          'type' => 'int',
          'unsigned' => TRUE,
          'not null' => TRUE,
          'description' => 'Ubercart order Line Item ID',
        ),
      ),
      MigrateDestinationEntityAPI::getKeySchema('commerce_line_item', 'product')
    );

    // Create a MigrateSource object, which manages retrieving the input data.
    $connection = commerce_migrate_ubercart_get_source_connection();

    $query = $connection->select('uc_order_line_items', 'uoli');
    $query->innerJoin('uc_orders', 'uo', 'uoli.order_id = uo.order_id');
    $query->condition('uo.order_status', 'in_checkout', '<>');
    $query->condition('uoli.type', 'shipping');
    $query->fields('uoli', array('line_item_id', 'order_id', 'title', 'amount'));
    $this->source = new MigrateSourceSQL($query, array(), NULL, array('map_joinable' => FALSE));
    $this->destination = new MigrateDestinationEntityAPI('commerce_line_item', 'shipping');

    // Properties.
    $this->addFieldMapping('order_id', 'order_id')
      ->sourceMigration('CommerceMigrateUbercartOrder');
    $this->addFieldMapping('line_item_label', 'title');
    $this->addFieldMapping('quantity')
      ->defaultValue(1);
    // Fields
    $this->addFieldMapping('commerce_unit_price', 'amount');
    $this->addFieldMapping('commerce_total', 'amount');
    $this->addFieldMapping('commerce_shipping_service', 'title')
      ->defaultValue(0);
  }

  /**
   * @param $line_item
   * @param stdClass $row
   */
  function prepare($line_item, stdClass $row) {
    $line_item->type = 'shipping';
  }


  /**
   * A line item has been saved. Reference it from the order.
   */
  function complete($line_item, stdClass $row) {

    $delta = db_query("SELECT COUNT(entity_id) FROM {field_data_commerce_line_items} WHERE entity_id = :order_id",
        array('order_id' => $line_item->order_id))->fetchField();

    db_insert('field_data_commerce_line_items')
    ->fields(array(
        'entity_id' => $line_item->order_id,
        'revision_id' => $line_item->order_id,
        'entity_type' => 'commerce_order',
        'bundle' => 'commerce_order',
        'deleted' => 0,
        'language' => LANGUAGE_NONE,
        'delta' => $delta,
        'commerce_line_items_line_item_id' => $line_item->line_item_id,
    ))
    ->execute();
  }

  /**
   * The line item has been deleted, delete its references.
   */
  function completeRollback($line_item_id) {
    db_delete('field_data_commerce_line_items')
    ->condition('commerce_line_items_line_item_id', $line_item_id)
    ->execute();
  }

}