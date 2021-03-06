/* File: verilog_netlist.c
 * 
 * This file is part of XSCHEM,
 * a schematic capture and Spice/Vhdl/Verilog netlisting tool for circuit 
 * simulation.
 * Copyright (C) 1998-2020 Stefan Frederik Schippers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "xschem.h"
static struct hashentry *subckt_table[HASHSIZE];

void global_verilog_netlist(int global)  /* netlister driver */
{
 FILE *fd;
 const char *str_tmp;
 char *sig_type = NULL;
 char *port_value = NULL;
 char *tmp_string=NULL;
 unsigned int *stored_flags;
 int i, tmp, save_ok;
 char netl_filename[PATH_MAX];  /* overflow safe 20161122 */
 char tcl_cmd_netlist[PATH_MAX + 100]; /* 20081203  overflow safe 20161122 */
 char cellname[PATH_MAX]; /* 20081203  overflow safe 20161122 */
 char *type=NULL;
 struct stat buf;
 char *subckt_name;

 if(modified) {
   save_ok = save_schematic(schematic[currentsch]);
   if(save_ok == -1) return;
 }
 free_hash(subckt_table);
 statusmsg("",2);  /* clear infowindow */
 netlist_count=0; 
 /* top sch properties used for library use declarations and type definitions */
 /* to be printed before any entity declarations */

 my_snprintf(netl_filename, S(netl_filename), "%s/.%s_%d", netlist_dir, skip_dir(schematic[currentsch]),getpid());
 fd=fopen(netl_filename, "w");

 if(user_top_netl_name[0]) {
   my_snprintf(cellname, S(cellname), "%s", get_cell(user_top_netl_name, 0));
 } else {
   my_snprintf(cellname, S(cellname), "%s.v", skip_dir(schematic[currentsch]));
 }

 if(fd==NULL){ 
   dbg(0, "global_verilog_netlist(): problems opening netlist file\n");
   return;
 }
 dbg(1, "global_verilog_netlist(): opening %s for writing\n",netl_filename);



/* print verilog timescale 10102004 */
 for(i=0;i<lastinst;i++)
 {
  if( strcmp(get_tok_value(inst_ptr[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
  if(inst_ptr[i].ptr<0) continue;
  if(!strcmp(get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
    continue;
  }
  my_strdup(105, &type,(inst_ptr[i].ptr+instdef)->type);
  if( type && (strcmp(type,"timescale")==0 || strcmp(type,"verilog_preprocessor")==0) )
  {
   str_tmp = get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr ,"format",0);
   my_strdup(106, &tmp_string, str_tmp);
   fprintf(fd, "%s\n", str_tmp ? translate(i, tmp_string) : "(NULL)");
  }
 }



 dbg(1, "global_verilog_netlist(): printing top level entity\n");
 fprintf(fd,"module %s (\n", skip_dir( schematic[currentsch]) );
 /* flush data structures (remove unused symbols) */
 unselect_all();
 remove_symbols();  /* removed 25122002, readded 04112003 */

 dbg(1, "global_verilog_netlist(): schematic[currentsch]=%s\n", schematic[currentsch]);
 
 load_schematic(1,schematic[currentsch] ,0);  /* 20180927 */


 /* print top subckt port directions */
 dbg(1, "global_verilog_netlist(): printing top level out pins\n");
 tmp=0;
 for(i=0;i<lastinst;i++)
 {
  if( strcmp(get_tok_value(inst_ptr[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
  if(inst_ptr[i].ptr<0) continue;
  if(!strcmp(get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
    continue;
  }
  my_strdup(546, &type,(inst_ptr[i].ptr+instdef)->type);
  if( type && (strcmp(type,"opin"))==0)
  {
   if(tmp) fprintf(fd, " ,\n"); 
   tmp++;
   str_tmp = get_tok_value(inst_ptr[i].prop_ptr,"lab",0);
   fprintf(fd, "  %s", str_tmp ? str_tmp : "(NULL)");
  }
 }

 dbg(1, "global_verilog_netlist(): printing top level inout pins\n");
 for(i=0;i<lastinst;i++)
 {
  if( strcmp(get_tok_value(inst_ptr[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
  if(inst_ptr[i].ptr<0) continue;
  if(!strcmp(get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
    continue;
  }
  my_strdup(547, &type,(inst_ptr[i].ptr+instdef)->type);
  if( type && (strcmp(type,"iopin"))==0)
  {
   if(tmp) fprintf(fd, " ,\n");
   tmp++;
   str_tmp = get_tok_value(inst_ptr[i].prop_ptr,"lab",0);
   fprintf(fd, "  %s", str_tmp ? str_tmp : "(NULL)");
  }
 }

 dbg(1, "global_verilog_netlist(): printing top level input pins\n");
 for(i=0;i<lastinst;i++)
 {
  if( strcmp(get_tok_value(inst_ptr[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
  if(inst_ptr[i].ptr<0) continue;
  if(!strcmp(get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
    continue;
  }
  my_strdup(548, &type,(inst_ptr[i].ptr+instdef)->type);
  if( type && (strcmp(type,"ipin"))==0)
  {
   if(tmp) fprintf(fd, " ,\n");
   tmp++;
   str_tmp = get_tok_value(inst_ptr[i].prop_ptr,"lab",0);
   fprintf(fd, "  %s", str_tmp ? str_tmp : "<NULL>");
  }
 }

 fprintf(fd,"\n);\n");

 /* 20071006 print top level params if defined in symbol */
 str_tmp = add_ext(schematic[currentsch], ".sym");
 if(!stat(str_tmp, &buf)) {
   load_sym_def(str_tmp, NULL );
   print_verilog_param(fd,lastinstdef-1);  /* added print top level params */
   remove_symbol(lastinstdef - 1);
 }
 /* 20071006 end */



 /* print top subckt port types */
 dbg(1, "global_verilog_netlist(): printing top level out pins\n");
 for(i=0;i<lastinst;i++)
 {
  if( strcmp(get_tok_value(inst_ptr[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
  if(inst_ptr[i].ptr<0) continue;
  if(!strcmp(get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
    continue;
  }
  my_strdup(549, &type,(inst_ptr[i].ptr+instdef)->type);
  if( type && (strcmp(type,"opin"))==0)
  {
   my_strdup(550, &port_value,get_tok_value(inst_ptr[i].prop_ptr,"value",0));
   my_strdup(551, &sig_type,get_tok_value(inst_ptr[i].prop_ptr,"verilog_type",0));
   if(!sig_type || sig_type[0]=='\0') my_strdup(552, &sig_type,"wire"); /* 20070720 changed reg to wire */
   str_tmp = get_tok_value(inst_ptr[i].prop_ptr,"lab",0);
   fprintf(fd, "  output %s ;\n", str_tmp ? str_tmp : "(NULL)");
   fprintf(fd, "  %s %s ", sig_type, str_tmp ? str_tmp : "(NULL)");
   /* 20140410 */
   if(port_value && port_value[0]) fprintf(fd," = %s", port_value);
   fprintf(fd, ";\n");
  }
 }

 dbg(1, "global_verilog_netlist(): printing top level inout pins\n");
 for(i=0;i<lastinst;i++)
 {
  if( strcmp(get_tok_value(inst_ptr[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
  if(inst_ptr[i].ptr<0) continue;
  if(!strcmp(get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
    continue;
  }
  my_strdup(553, &type,(inst_ptr[i].ptr+instdef)->type);
  if( type && (strcmp(type,"iopin"))==0)
  {
   my_strdup(554, &port_value,get_tok_value(inst_ptr[i].prop_ptr,"value",0));
   my_strdup(555, &sig_type,get_tok_value(inst_ptr[i].prop_ptr,"verilog_type",0));
   if(!sig_type || sig_type[0]=='\0') my_strdup(556, &sig_type,"wire");
   str_tmp = get_tok_value(inst_ptr[i].prop_ptr,"lab",0);
   fprintf(fd, "  inout %s ;\n", str_tmp ? str_tmp : "(NULL)");
   fprintf(fd, "  %s %s ", sig_type, str_tmp ? str_tmp : "(NULL)");
   /* 20140410 */
   if(port_value && port_value[0]) fprintf(fd," = %s", port_value);
   fprintf(fd, ";\n");
  }
 }

 dbg(1, "global_verilog_netlist(): printing top level input pins\n");
 for(i=0;i<lastinst;i++)
 {
  if( strcmp(get_tok_value(inst_ptr[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
  if(inst_ptr[i].ptr<0) continue;
  if(!strcmp(get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
    continue;
  }
  my_strdup(557, &type,(inst_ptr[i].ptr+instdef)->type);
  if( type && (strcmp(type,"ipin"))==0)
  {
   my_strdup(558, &port_value,get_tok_value(inst_ptr[i].prop_ptr,"value",0));
   my_strdup(559, &sig_type,get_tok_value(inst_ptr[i].prop_ptr,"verilog_type",0));
   if(!sig_type || sig_type[0]=='\0') my_strdup(560, &sig_type,"wire");
   str_tmp = get_tok_value(inst_ptr[i].prop_ptr,"lab",0);
   fprintf(fd, "  input %s ;\n", str_tmp ? str_tmp : "<NULL>");
   fprintf(fd, "  %s %s ", sig_type, str_tmp ? str_tmp : "<NULL>");
   /* 20140410 */
   if(port_value && port_value[0]) fprintf(fd," = %s", port_value);
   fprintf(fd, ";\n");
  }
 }

 dbg(1, "global_verilog_netlist(): netlisting  top level\n");
 verilog_netlist(fd, 0);
 netlist_count++;
 fprintf(fd,"---- begin user architecture code\n");
 /* 20180124 */
 for(i=0;i<lastinst;i++) {
   if( strcmp(get_tok_value(inst_ptr[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
   if(inst_ptr[i].ptr<0) continue;
   if(!strcmp(get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
     continue;
   }
   my_strdup(561, &type,(inst_ptr[i].ptr+instdef)->type);
   if(type && !strcmp(type,"netlist_commands")) {
     fprintf(fd, "%s\n", get_tok_value(inst_ptr[i].prop_ptr,"value",2)); /* 20180124 */
   }
 }


 if(schverilogprop && schverilogprop[0]) {
   fprintf(fd, "%s\n", schverilogprop); 
 }
 fprintf(fd,"---- end user architecture code\n");
 fprintf(fd, "endmodule\n");

 if(split_files) { /* 20081205 */
   fclose(fd);
   my_snprintf(tcl_cmd_netlist, S(tcl_cmd_netlist), "netlist {%s} noshow {%s}", netl_filename, cellname);
   tcleval(tcl_cmd_netlist);
   if(debug_var==0) xunlink(netl_filename);
 }

 /* preserve current level instance flags before descending hierarchy for netlisting, restore later */
 stored_flags = my_calloc(150, lastinst, sizeof(unsigned int));
 for(i=0;i<lastinst;i++) stored_flags[i] = inst_ptr[i].flags & 4;

 if(global)
 {
   unselect_all();
   remove_symbols(); /* 20161205 ensure all unused symbols purged before descending hierarchy */
   
   load_schematic(1, schematic[currentsch], 0); /* 20180927 */

   currentsch++;
   dbg(2, "global_verilog_netlist(): last defined symbol=%d\n",lastinstdef);
   subckt_name=NULL;
   for(i=0;i<lastinstdef;i++)
   {
    if( strcmp(get_tok_value(instdef[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20070726 */
    if(!instdef[i].type) continue;
    if(strcmp(instdef[i].type,"subcircuit")==0 && check_lib(instdef[i].name)) {
      /* instdef can be SCH or SYM, use hash to avoid writing duplicate subckt */
      my_strdup(328, &subckt_name, get_cell(instdef[i].name, 0));
      if (hash_lookup(subckt_table, subckt_name, "", XLOOKUP)==NULL)
      {
        hash_lookup(subckt_table, subckt_name, "", XINSERT);
        if( split_files && strcmp(get_tok_value(instdef[i].prop_ptr,"vhdl_netlist",0),"true")==0 )
          vhdl_block_netlist(fd, i); /* 20081209 */
        else if(split_files && strcmp(get_tok_value(instdef[i].prop_ptr,"spice_netlist",0),"true")==0 )
          spice_block_netlist(fd, i); /* 20081209 */
        else 
          if( strcmp(get_tok_value(instdef[i].prop_ptr,"verilog_primitive",0), "true")) 
            verilog_block_netlist(fd, i); /* 20081205 */
      }
    }
   }
   free_hash(subckt_table);
   my_free(1073, &subckt_name);
   my_strncpy(schematic[currentsch] , "", S(schematic[currentsch]));
   currentsch--;
   unselect_all();
   /* remove_symbols(); */
   load_schematic(1,schematic[currentsch], 0);
   prepare_netlist_structs(1); /* so 'lab=...' attributes for unnamed nets are set */
   /* symbol vs schematic pin check, we do it here since now we have ALL symbols loaded */
   sym_vs_sch_pins();

   /* restore hilight flags from errors found analyzing top level before descending hierarchy */
   for(i=0;i<lastinst; i++) inst_ptr[i].flags |= stored_flags[i];

   draw_hilight_net(1);
 }
 my_free(1074, &stored_flags);

 dbg(1, "global_verilog_netlist(): starting awk on netlist!\n");
 if(!split_files) {
   fclose(fd);
   if(netlist_show) {
    my_snprintf(tcl_cmd_netlist, S(tcl_cmd_netlist), "netlist {%s} show {%s}", netl_filename, cellname);
    tcleval(tcl_cmd_netlist);
   }
   else {
    my_snprintf(tcl_cmd_netlist, S(tcl_cmd_netlist), "netlist {%s} noshow {%s}", netl_filename, cellname);
    tcleval(tcl_cmd_netlist);
   }
   if(debug_var == 0 ) xunlink(netl_filename);
 }
 my_free(1075, &sig_type);
 my_free(1076, &port_value);
 my_free(1077, &tmp_string);
 my_free(1078, &type);
}


void verilog_block_netlist(FILE *fd, int i)  /*20081205 */
{
  int j, l, tmp;
  int verilog_stop=0;
  char *dir_tmp = NULL;
  char *sig_type = NULL;
  char *port_value = NULL;
  char *type = NULL;
  char *tmp_string = NULL;
  char filename[PATH_MAX];
  char netl_filename[PATH_MAX];
  char tcl_cmd_netlist[PATH_MAX + 100];  /* 20081202 */
  char cellname[PATH_MAX];  /* 20081202 */
  const char *str_tmp;

  if(!strcmp( get_tok_value(instdef[i].prop_ptr,"verilog_stop",0),"true") ) 
     verilog_stop=1;
  else
     verilog_stop=0;


  if(split_files) {          /* 20081203 */
    my_snprintf(netl_filename, S(netl_filename), "%s/.%s_%d", netlist_dir,  skip_dir(instdef[i].name), getpid());
    dbg(1, "global_vhdl_netlist(): split_files: netl_filename=%s\n", netl_filename);
    fd=fopen(netl_filename, "w");
    my_snprintf(cellname, S(cellname), "%s.v", skip_dir(instdef[i].name) );

  }

  dbg(1, "verilog_block_netlist(): expanding %s\n",  instdef[i].name);
  fprintf(fd, "\n// expanding   symbol:  %s # of pins=%d\n\n", 
        instdef[i].name,instdef[i].rects[PINLAYER] );


  if((str_tmp = get_tok_value(instdef[i].prop_ptr, "schematic",0 ))[0]) {
    my_strncpy(filename, abs_sym_path(str_tmp, ""), S(filename));
    load_schematic(1,filename, 0);
  } else {
    verilog_stop? load_schematic(0, add_ext(abs_sym_path(instdef[i].name, ""), ".sch"), 0) :   /* 20190518 */
                  load_schematic(1, add_ext(abs_sym_path(instdef[i].name, ""), ".sch"), 0);
  }


  /* print verilog timescale  and preprocessor directives 10102004 */

   for(j=0;j<lastinst;j++)
   {
    if( strcmp(get_tok_value(inst_ptr[j].prop_ptr,"verilog_ignore",0),"true")==0 ) continue;
    if(inst_ptr[j].ptr<0) continue;
    if(!strcmp(get_tok_value( (inst_ptr[j].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
      continue;
    }
    my_strdup(544, &type,(inst_ptr[j].ptr+instdef)->type);
    if( type && ( strcmp(type,"timescale")==0  || strcmp(type,"verilog_preprocessor")==0) )
    {
     str_tmp = get_tok_value( (inst_ptr[j].ptr+instdef)->prop_ptr ,"format",0);
     my_strdup(545, &tmp_string, str_tmp);
     fprintf(fd, "%s\n", str_tmp ? translate(j, tmp_string) : "(NULL)");
    }
   }

  fprintf(fd, "module %s (\n", skip_dir(instdef[i].name));
  /*print_generic(fd, "entity", i); */ /* 02112003 */

  dbg(1, "verilog_block_netlist():       entity ports\n");

  /* print ports directions */
  tmp=0;
  for(j=0;j<instdef[i].rects[PINLAYER];j++)
  {
    if(strcmp(get_tok_value(instdef[i].boxptr[PINLAYER][j].prop_ptr,"verilog_ignore",0), "true")) {
      str_tmp = get_tok_value(instdef[i].boxptr[PINLAYER][j].prop_ptr,"name",0);
      if(tmp) fprintf(fd, " ,\n"); 
      tmp++;
      fprintf(fd,"  %s", str_tmp ? str_tmp : "<NULL>");
    }
  }
  fprintf(fd, "\n);\n");

  /*16112003 */
  dbg(1, "verilog_block_netlist():       entity generics\n");
  /* print module  default parameters */
  print_verilog_param(fd,i);




  /* print port types */
  tmp=0;
  for(j=0;j<instdef[i].rects[PINLAYER];j++)
  {
    if(strcmp(get_tok_value(instdef[i].boxptr[PINLAYER][j].prop_ptr,"verilog_ignore",0), "true")) {
      my_strdup(564, &sig_type,get_tok_value(
                instdef[i].boxptr[PINLAYER][j].prop_ptr,"verilog_type",0));
      my_strdup(565, &port_value, get_tok_value(
                instdef[i].boxptr[PINLAYER][j].prop_ptr,"value",2) );
      my_strdup(566, &dir_tmp, get_tok_value(instdef[i].boxptr[PINLAYER][j].prop_ptr,"dir",0) );
      if(strcmp(dir_tmp,"in")){
         if(!sig_type || sig_type[0]=='\0') my_strdup(567, &sig_type,"wire"); /* 20070720 changed reg to wire */
      } else {
         if(!sig_type || sig_type[0]=='\0') my_strdup(568, &sig_type,"wire");
      }
      str_tmp = get_tok_value(instdef[i].boxptr[PINLAYER][j].prop_ptr,"name",0);
      fprintf(fd,"  %s %s ;\n", 
        strcmp(dir_tmp,"in")? ( strcmp(dir_tmp,"out")? "inout" :"output"  ) : "input",
        str_tmp ? str_tmp : "<NULL>");
      fprintf(fd,"  %s %s", 
        sig_type, 
        str_tmp ? str_tmp : "<NULL>");
      if(port_value &&port_value[0])
        fprintf(fd," = %s", port_value);
      fprintf(fd," ;\n");
    }
  }

  dbg(1, "verilog_block_netlist():       netlisting %s\n", skip_dir( schematic[currentsch]));
  verilog_netlist(fd, verilog_stop);
  netlist_count++;
  fprintf(fd,"---- begin user architecture code\n");
  for(l=0;l<lastinst;l++) {
    if( strcmp(get_tok_value(inst_ptr[l].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
    if(inst_ptr[l].ptr<0) continue;
    if(!strcmp(get_tok_value( (inst_ptr[l].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) {
      continue;
    }
    if(netlist_count &&
      !strcmp(get_tok_value(inst_ptr[l].prop_ptr, "only_toplevel", 0), "true")) continue; /* 20160418 */

    my_strdup(569, &type,(inst_ptr[l].ptr+instdef)->type);
    if(type && !strcmp(type,"netlist_commands")) {
      fprintf(fd, "%s\n", get_tok_value(inst_ptr[l].prop_ptr,"value",2)); /* 20180124 */
    }
  }

  if(schverilogprop && schverilogprop[0]) {
    fprintf(fd, "%s\n", schverilogprop);
  }
  fprintf(fd,"---- end user architecture code\n");
  fprintf(fd, "endmodule\n");
  if(split_files) { /* 20081204 */
    fclose(fd);
    my_snprintf(tcl_cmd_netlist, S(tcl_cmd_netlist), "netlist {%s} noshow {%s}", netl_filename, cellname);
    tcleval(tcl_cmd_netlist);
    if(debug_var==0) xunlink(netl_filename);
  }
  my_free(1079, &dir_tmp);
  my_free(1080, &sig_type);
  my_free(1081, &port_value);
  my_free(1082, &type);
  my_free(1083, &tmp_string);
}

void verilog_netlist(FILE *fd , int verilog_stop)
{
 int i;
 char *type=NULL;

 prepared_netlist_structs = 0;
 prepare_netlist_structs(1);
 /* set_modify(1); */ /* 20160302 prepare_netlist_structs could change schematic (wire node naming for example) */
 dbg(2, "verilog_netlist(): end prepare_netlist_structs\n");
 traverse_node_hash();  /* print all warnings about unconnected floatings etc */

 dbg(2, "verilog_netlist(): end traverse_node_hash\n");

 fprintf(fd,"---- begin signal list\n");
 if(!verilog_stop) print_verilog_signals(fd);
 fprintf(fd,"---- end signal list\n");


 if(!verilog_stop)
 {
   for(i=0;i<lastinst;i++) /* ... print all element except ipin opin labels use package */
   {
    if( strcmp(get_tok_value(inst_ptr[i].prop_ptr,"verilog_ignore",0),"true")==0 ) continue; /* 20140416 */
    if(inst_ptr[i].ptr<0) continue;
    if(!strcmp(get_tok_value( (inst_ptr[i].ptr+instdef)->prop_ptr, "verilog_ignore",0 ), "true") ) { /*20070726 */
      continue;                                                                                   /*20070726 */
    }                                                                                             /*20070726 */

    dbg(2, "verilog_netlist():       into the netlisting loop\n");
    my_strdup(570, &type,(inst_ptr[i].ptr+instdef)->type);
    if( type && 
       ( strcmp(type,"label")&&
         strcmp(type,"ipin")&&
         strcmp(type,"opin")&&
         strcmp(type,"iopin")&&
         strcmp(type,"netlist_commands")&& /* 20180124 */
         strcmp(type,"timescale")&&
         strcmp(type,"verilog_preprocessor")
       ))
    {
     if(lastselected) 
     {
      if(inst_ptr[i].sel==SELECTED) print_verilog_element(fd, i) ;
     }
     else print_verilog_element(fd, i) ;  /* this is the element line  */
    }
   }
   my_free(1084, &type);
 }
 dbg(1, "verilog_netlist():       end\n");
 if(!netlist_count) redraw_hilights(); /*draw_hilight_net(1); */
}
