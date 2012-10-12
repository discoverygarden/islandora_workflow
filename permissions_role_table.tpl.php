<?php
/**
 * @file
 *   The template for permissions by role.
 */

  print($list['collection_selector'] . $list['collection_submit']);
?>
<TABLE>

  <THEAD>
    <TR>
      <TH>
      		Role
      </TH>
      <th>
        <?php print $list['active_collection_label'] . ' [' . $list['active_collection_pid'] . ']'; ?>
      </th>
      <!-- Display a header for each collection -->
      <?php
      //print('<TH>' . $_SESSION['workflow_permissions_page']['active_collection'] . '</TH>');
      ?>
    </TR>
	</THEAD>

	<TFOOT>
	</TFOOT>

	<TBODY>
      <!-- Display Table cells for each collection/role intersection -->
      <?php
        foreach($roles as $role_id => $role_name) :
          if ($list[$role_id][$_SESSION['workflow_permissions_page']['active_collection']]) :
            print('<TR><TD>' . key($role_name) . '</TD>');
            print('<TD>' . $list[$role_id][$_SESSION['workflow_permissions_page']['active_collection']] . '</TD>');
            print('</TR>');
          endif;
        endforeach;
      ?>
	</TBODY>

</TABLE>
