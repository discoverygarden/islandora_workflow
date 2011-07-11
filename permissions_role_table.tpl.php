<?php 
  print($list['collection_selector'].$list['collection_submit']);
?>
<TABLE>

  <THEAD>
    <TR>
      <TH>
      		Roles
      </TH>
      <!-- Display a header for each collection -->
      <?php 
        /*foreach($collections as $collection_id => $collection_name) {
          print('<TH>' . $collection_id .'</BR>' . $collection_name .'</TH>');
        }*/
      print('<TH>' . $_SESSION['workflow_permissions_page']['active_collection'] .'</TH>');
      ?>
    </TR>
	</THEAD>
	
	<TFOOT>
	</TFOOT>
	
	<TBODY>
	<!-- Display Table cells for each collection/role intersection -->
		<?php 
		  foreach($roles as $role_id => $role_name) {
		    print('<TR><TD>'.key($role_name).'</TD>');
		    print('<TD>'.$list[$role_id][$_SESSION['workflow_permissions_page']['active_collection']].'</TD>');
		    /*foreach($collections as $collection_id =>$collection_name) {
		      print('<TD>'.$list[$role_id][$collection_id].'</TD>');
		    }*/
		    print('</TR>');
		  }
		  ?>
	</TBODY>
	
</TABLE>