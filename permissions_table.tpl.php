<TABLE>

  <THEAD>
    <TR>
      <TH>
      		Roles
      </TH>
      <!-- Display a header for each collection -->
      <?php 
        /*foreach($collections as $collection) {
          print(<TR>$collection</TR>);
        }*/
      ?>
    </TR>
	</THEAD>
	
	<TFOOT>
	</TFOOT>
	
	<TBODY>
	<!-- Display Table cells for each collection/role intersection -->
		<?php 
		  
		  print('<TR><TD>'.$table_elems.'</TD></TR>');
		  
		  ?>
	</TBODY>
	
</TABLE>