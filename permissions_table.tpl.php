<TABLE>

  <THEAD>
    <TR>
      <TH>
      		Roles
      </TH>
      <!-- Display a header for each collection -->
      <?php 
        foreach($collections as $collection) {
          print('<TR>'.$collection.'</TR>');
        }
      ?>
    </TR>
	</THEAD>
	
	<TFOOT>
	</TFOOT>
	
	<TBODY>
	<!-- Display Table cells for each collection/role intersection -->
		<?php 
		  foreach($roles as $role) {
		    print('<TR><TD>'.$role.'</TD>');
		  
		    foreach($collections as $collection) {
		      print('<TD>'.$table_elems.'</TD>');
		    }
		    print('</TR>');
		  }
		  ?>
	</TBODY>
	
</TABLE>