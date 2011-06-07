<TABLE>

  <THEAD>
    <TR>
      <TH>
      		Roles
      </TH>
      <!-- Display a header for each collection -->
      <?php 
        foreach($collections as $collection) {
          print('<TH>'.key($collections).'</BR>'.$collection.'</TH>');
        }
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
		    foreach($collections as $collection_id) {
		      print('<TD>'.$list[$role_id][key($collections)].'</TD>');
		    }
		    print('</TR>');
		  }
		  ?>
	</TBODY>
	
</TABLE>