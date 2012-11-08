<?php
/**
 * @file
 *   Template for the permissions by user table.
 */

  print($list['collection_selector'] . $list['collection_submit']);
?>
<TABLE>

  <THEAD>
    <TR>
      <TH>
        User
      </TH>
      <th>
        <?php print $list['active_collection_label'] . ' [' . $list['active_collection_pid'] . ']'; ?>
      </th>
    </TR>
	</THEAD>

  <TFOOT>
  </TFOOT>

  <TBODY>
    <!-- Display Table cells for each collection/role intersection -->
    <?php
      foreach($users as $user_id => $user_name) :
        print('<TR><TD>' . key($user_name) . '</TD>');
        print('<TD>' . $list[$user_id][$_SESSION['workflow_permissions_page']['active_collection']] . '</TD>');
        print('</TR>');
      endforeach;
    ?>
  </TBODY>

</TABLE>
