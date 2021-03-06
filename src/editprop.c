/* File: editprop.c
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

#include <stdarg.h>
#include "xschem.h"

static int rot = 0, flip = 0;          

char *my_strtok_r(char *str, const char *delim, char **saveptr)
{
  char *tok;
  if(str) { /* 1st call */
    *saveptr = str;
  }
  while(**saveptr && strchr(delim, **saveptr) ) { /* skip separators */
    ++(*saveptr);
  }
  tok = *saveptr; /* start of token */
  while(**saveptr && !strchr(delim, **saveptr) ) { /* look for separator marking end of current token */
    ++(*saveptr);
  }
  if(**saveptr) {
    **saveptr = '\0'; /* mark end of token */
    ++(*saveptr);     /* if not at end of string advance one char for next iteration */
  }
  if(tok[0]) return tok; /* return token */
  else return NULL; /* no more tokens */
}

size_t my_strdup(int id, char **dest, const char *src) /* empty source string --> dest=NULL */
{
 size_t len;

 if(src!=NULL && src[0]!='\0')  {
   len = strlen(src)+1;
   my_realloc(id, dest, len);
   memcpy(*dest, src, len);
   dbg(3,"my_strdup(%d,): duplicated string %s\n", id, src);
   return len-1;
 } else if(*dest) {
   my_free(1146, dest);
   dbg(3,"my_strdup(%d,): freed destination ptr\n", id);
 }
 
 return 0;
}

/* 20171004 copy at most n chars, adding a null char at end */
void my_strndup(int id, char **dest, const char *src, int n) /* empty source string --> dest=NULL */

{
 if(*dest!=NULL) {
   dbg(3,"  my_strndup:  calling my_free\n");
   my_free(1147, dest);
 }
 if(src!=NULL && src[0]!='\0')
 {
  /* 20180924 replace strndup() */
  char *p = memchr(src, '\0', n);
  if(p) n = p - src;
  *dest = my_malloc(id, n+1);
  if(*dest) {
    memcpy(*dest, src, n);
    (*dest)[n] = '\0';
  }
  /* *dest=strndup(src, n); */

  dbg(3,"my_strndup(%d,): duplicated string %s\n", id, src);
 }
}

void dbg(int level, char *fmt, ...)
{
  if(debug_var>=level) {
    va_list args;
    va_start(args, fmt);
    vfprintf(errfp, fmt, args);
    va_end(args);
  }
}

#ifdef HAS_SNPRINTF
int my_snprintf(char *str, int size, const char *fmt, ...) /* 20161124 */
{
  int  size_of_print;
  char s[200];

  va_list args;
  va_start(args, fmt);
  size_of_print = vsnprintf(str, size, fmt, args);

  if(has_x && size_of_print >=size) { /* output was truncated  */
    snprintf(s, S(s), "alert_ { Warning: overflow in my_snprintf print size=%d, buffer size=%d} {}",
             size_of_print, size);
    tcleval(s);
  }
  va_end(args);
  return size_of_print;
}
#else

/*
   this is a replacement for snprintf(), **however** it implements only
   the bare minimum set of formatting used by XSCHEM
*/
int my_snprintf(char *string, int size, const char *format, ...)
{
  va_list args;
  const char *f, *fmt = NULL, *prev;
  int overflow, format_spec, l, n = 0;

  va_start(args, format);

  /* fprintf(errfp, "my_snprintf(): size=%d, format=%s\n", size, format); */
  prev = format;
  format_spec = 0;
  overflow = 0;
  for(f = format; *f; f++) {
    if(*f == '%') {
      format_spec = 1;
      fmt = f;
    }
    if(*f == 's' && format_spec) {
      char *sptr;
      sptr = va_arg(args, char *);
      l = fmt - prev;
      if(n+l > size) {
        overflow = 1;
        break;
      }
      memcpy(string + n, prev, l);
      string[n+l] = '\0';
      n += l;
      l = strlen(sptr);
      if(n+l+1 > size) {
        overflow = 1;
        break;
      }
      memcpy(string + n, sptr, l+1);
      n += l;
      format_spec = 0;
      prev = f + 1;
    }
    else if(format_spec && (*f == 'd' || *f == 'x' || *f == 'c') ) {
      char nfmt[50], nstr[50];
      int i, nlen;
      i = va_arg(args, int);
      l = f - fmt+1;
      strncpy(nfmt, fmt, l);
      nfmt[l] = '\0';
      l = fmt - prev;
      if(n+l > size) break;
      memcpy(string + n, prev, l);
      string[n+l] = '\0';
      n += l;
      nlen = sprintf(nstr, nfmt, i);
      if(n + nlen + 1 > size) {
        overflow = 1;
        break;
      }
      memcpy(string +n, nstr, nlen+1);
      n += nlen;
      format_spec = 0;
      prev = f + 1;
    }
    else if(format_spec && *f == 'g') {
      char nfmt[50], nstr[50];
      double i;
      int nlen;
      i = va_arg(args, double);
      l = f - fmt+1;
      strncpy(nfmt, fmt, l);
      nfmt[l] = '\0';
      l = fmt - prev;
      if(n+l > size) {
        overflow = 1;
        break;
      }
      memcpy(string + n, prev, l);
      string[n+l] = '\0';
      n += l;
      nlen = sprintf(nstr, nfmt, i);
      if(n + nlen + 1 > size) {
        overflow = 1;
        break;
      }
      memcpy(string +n, nstr, nlen+1);
      n += nlen;
      format_spec = 0;
      prev = f + 1;
    }
  }
  l = f - prev;
  if(!overflow && n+l+1 <= size) {
    memcpy(string + n, prev, l+1);
    n += l;
  } else {
    dbg(1, "my_snprintf(): overflow, target size=%d, format=%s\n", size, format);
  }
  
  va_end(args);
  /* fprintf(errfp, "my_snprintf(): returning: |%s|\n", string); */
  return n;
}
#endif /* HAS_SNPRINTF */

size_t my_strdup2(int id, char **dest, const char *src) /* 20150409 duplicates also empty string  */
{
 size_t len;
 if(src!=NULL) {
   len = strlen(src)+1;
   my_realloc(id, dest, len);
   memcpy(*dest, src, len);
   dbg(3,"my_strdup2(%d,): duplicated string %s\n", id, src);
   return len-1;
 } else if(*dest) {
   my_free(1148, dest);
   dbg(3,"my_strdup2(%d,): freed destination ptr\n", id);
 }
 return 0;
}

size_t my_strcat(int id, char **str, const char *append_str)
{
  size_t s, a;
  dbg(3,"my_strcat(%d,): str=%s  append_str=%s\n", id, *str, append_str);
  if( *str != NULL)
  {
    s = strlen(*str);
    if(append_str == NULL || append_str[0]=='\0') return s;
    a = strlen(append_str) + 1;
    my_realloc(id, str, s + a );
    memcpy(*str + s, append_str, a); /* 20180923 */
    dbg(3,"my_strcat(%d,): reallocated string %s\n", id, *str);
    return s + a - 1;
  } else {
    if(append_str == NULL || append_str[0] == '\0') return 0;
    a = strlen(append_str) + 1;
    *str = my_malloc(id, a);
    memcpy(*str, append_str, a); /* 20180923 */
    dbg(3,"my_strcat(%d,): allocated string %s\n", id, *str);
    return a - 1;
  }
}

size_t my_strncat(int id, char **str, size_t n, const char *append_str)
{
 size_t s, a;
 dbg(3,"my_strncat(%d,): str=%s  append_str=%s\n", id, *str, append_str);
 a = strlen(append_str)+1;
 if(a>n+1) a=n+1;
 if( *str != NULL)
 {
  s = strlen(*str);
  if(append_str == NULL || append_str[0]=='\0') return s;
  my_realloc(id, str, s + a );
  memcpy(*str+s, append_str, a); /* 20180923 */
  *(*str+s+a) = '\0';
  dbg(3,"my_strncat(%d,): reallocated string %s\n", id, *str);
  return s + a -1;
 }
 else
 {
  if(append_str == NULL || append_str[0]=='\0') return 0;
  *str = my_malloc(id,  a );
  memcpy(*str, append_str, a); /* 20180923 */
  *(*str+a) = '\0';
  dbg(3,"my_strncat(%d,): allocated string %s\n", id, *str);
  return a -1;
 }
}


void *my_calloc(int id, size_t nmemb, size_t size)
{
   void *ptr;
   if(size*nmemb > 0) {
     ptr=calloc(nmemb, size);
     if(ptr == NULL) fprintf(errfp,"my_calloc(%d,): allocation failure\n", id);
     dbg(3, "my_calloc(%d,): allocating %p , %lu bytes\n",
               id, ptr, (unsigned long) (size*nmemb));
   }
   else ptr = NULL;
   return ptr;
}

void *my_malloc(int id, size_t size) 
{
 void *ptr;
 if(size>0) {
   ptr=malloc(size);
  if(ptr == NULL) fprintf(errfp,"my_malloc(%d,): allocation failure\n", id);
   dbg(3, "my_malloc(%d,): allocating %p , %lu bytes\n",
     id, ptr, (unsigned long) size);
 }
 else ptr=NULL;
 return ptr;
}

void my_realloc(int id, void *ptr,size_t size)
{
 void *a;
 a = *(void **)ptr;
 if(size == 0) {
   free(*(void **)ptr);
   dbg(3, "my_free(%d,):  my_realloc_freeing %p\n",id, *(void **)ptr);
   *(void **)ptr=NULL;
 } else {
   *(void **)ptr=realloc(*(void **)ptr,size);
    if(*(void **)ptr == NULL) fprintf(errfp,"my_realloc(%d,): allocation failure\n", id);
   dbg(3, "my_realloc(%d,): reallocating %p --> %p to %lu bytes\n",
           id, a, *(void **)ptr,(unsigned long) size);
 }

} 

void my_free(int id, void *ptr)
{
 if(*(void **)ptr) {
   free(*(void **)ptr);
   dbg(3, "my_free(%d,):  freeing %p\n", id, *(void **)ptr);
   *(void **)ptr=NULL;
 } else {
   dbg(3, "--> my_free(%d,): trying to free NULL pointer\n", id);
 }
}

/* n characters at most are copied, *d will be always NUL terminated if *s does
 *   not fit(d[n-1]='\0')
 * return # of copied characters
 */
int my_strncpy(char *d, const char *s, int n)
{
  int i = 0;
  n -= 1;
  dbg(3, "my_strncpy():  copying %s to %lu\n", s, (unsigned long)d);
  while( (d[i] = s[i]) )
  {
    if(i == n) { 
      if(s[i] != '\0') dbg(1, "my_strncpy(): overflow, n=%d, s=%s\n", n+1, s);
      d[i] = '\0';
      return i; 
    }
    i++;
  }
  return i;
}

char *strtolower(char* s) {
  char *p;
  for(p=s; *p; p++) *p=tolower(*p);
  return s;
}
char *strtoupper(char* s) {
  char *p;
  for(p=s; *p; p++) *p=toupper(*p);
  return s;
}


void set_inst_prop(int i)
{
  char *ptr;
  char *tmp = NULL;

  ptr = (inst_ptr[i].ptr+instdef)->templ; /*20150409 */
  dbg(1, "set_inst_prop(): i=%d, name=%s, prop_ptr = %s, template=%s\n", 
     i, inst_ptr[i].name, inst_ptr[i].prop_ptr, ptr);
  my_strdup(69, &inst_ptr[i].prop_ptr, ptr);
  my_strdup2(70, &inst_ptr[i].instname, get_tok_value(ptr, "name",0)); /* 20150409 */
  if(inst_ptr[i].instname[0]) {
    my_strdup(101, &tmp, inst_ptr[i].prop_ptr);
    new_prop_string(i, tmp, 0, disable_unique_names);
    my_free(724, &tmp);
  }
}

void edit_rect_property(void) 
{
  int i, c, n, old_dash;
  int drw = 0;
  const char *dash;
  int preserve;
  char *oldprop=NULL;
  if(rect[selectedgroup[0].col][selectedgroup[0].n].prop_ptr!=NULL) {
    my_strdup(67, &oldprop, rect[selectedgroup[0].col][selectedgroup[0].n].prop_ptr);
    tclsetvar("retval",oldprop);
  } else { /* 20161208 */
    tclsetvar("retval","");
  }

  tcleval("text_line {Input property:} 0 normal");
  preserve = atoi(tclgetvar("preserve_unchanged_attrs"));
  if(strcmp(tclgetvar("rcode"),"") )
  {
    push_undo();
    set_modify(1);
    for(i=0; i<lastselected; i++) {
      if(selectedgroup[i].type != xRECT) continue;
      c = selectedgroup[i].col;
      n = selectedgroup[i].n;
      if(preserve == 1) {
        set_different_token(&rect[c][n].prop_ptr, 
               (char *) tclgetvar("retval"), oldprop, 0, 0);
      } else {
        my_strdup(99, &rect[c][n].prop_ptr,
               (char *) tclgetvar("retval"));
      }
      old_dash = rect[c][n].dash;
      dash = get_tok_value(rect[c][n].prop_ptr,"dash",0);
      if( strcmp(dash, "") ) {
        int d = atoi(dash);
        rect[c][n].dash = d >= 0? d : 0;
      } else
        rect[c][n].dash = 0;
      if(old_dash != rect[c][n].dash) {
         if(!drw) {
           bbox(BEGIN,0.0,0.0,0.0,0.0);
           drw = 1;
         }
         bbox(ADD, rect[c][n].x1, rect[c][n].y1, rect[c][n].x2, rect[c][n].y2);
      }
    }
    if(drw) { 
      bbox(SET , 0.0 , 0.0 , 0.0 , 0.0);
      draw();
      bbox(END , 0.0 , 0.0 , 0.0 , 0.0);
    }
  }
  my_free(725, &oldprop);
}


void edit_line_property(void)
{
  int i, c, n;
  const char *dash;
  int preserve;
  char *oldprop=NULL;
  if(line[selectedgroup[0].col][selectedgroup[0].n].prop_ptr!=NULL) {
    my_strdup(46, &oldprop, line[selectedgroup[0].col][selectedgroup[0].n].prop_ptr);
    tclsetvar("retval", oldprop);
  } else { /* 20161208 */
    tclsetvar("retval","");
  }
  tcleval("text_line {Input property:} 0 normal");
  preserve = atoi(tclgetvar("preserve_unchanged_attrs"));
  if(strcmp(tclgetvar("rcode"),"") )
  {
    int y1, y2;
    push_undo();
    set_modify(1);
    bbox(BEGIN, 0.0 , 0.0 , 0.0 , 0.0);
    for(i=0; i<lastselected; i++) {
      if(selectedgroup[i].type != LINE) continue;
      c = selectedgroup[i].col;
      n = selectedgroup[i].n;
      if(preserve == 1) {
        set_different_token(&line[c][n].prop_ptr, 
               (char *) tclgetvar("retval"), oldprop, 0, 0);
      } else {
        my_strdup(102, &line[c][n].prop_ptr,
               (char *) tclgetvar("retval"));
      }
      line[c][n].bus = !strcmp(get_tok_value(line[c][n].prop_ptr,"bus",0), "true");
      dash = get_tok_value(line[c][n].prop_ptr,"dash",0);
      if( strcmp(dash, "") ) {
        int d = atoi(dash);
        line[c][n].dash = d >= 0? d : 0;
      } else
        line[c][n].dash = 0;
      if(line[c][n].y1 < line[c][n].y2) { y1 = line[c][n].y1-bus_width; y2 = line[c][n].y2+bus_width; }
      else                        { y1 = line[c][n].y1+bus_width; y2 = line[c][n].y2-bus_width; }
      bbox(ADD, line[c][n].x1-bus_width, y1 , line[c][n].x2+bus_width , y2 );
    }
    bbox(SET , 0.0 , 0.0 , 0.0 , 0.0);
    draw();
    bbox(END , 0.0 , 0.0 , 0.0 , 0.0);
  }
  my_free(726, &oldprop);
}


void edit_wire_property(void)
{
  int i;
  int preserve;
  char *oldprop=NULL;
  const char *bus_ptr;

  if(wire[selectedgroup[0].n].prop_ptr!=NULL) {
    my_strdup(47, &oldprop, wire[selectedgroup[0].n].prop_ptr);
    tclsetvar("retval", oldprop);
  } else { /* 20161208 */
    tclsetvar("retval","");
  }
  tcleval("text_line {Input property:} 0 normal");
  preserve = atoi(tclgetvar("preserve_unchanged_attrs"));
  if(strcmp(tclgetvar("rcode"),"") )
  {
    push_undo(); /* 20150327 */
    set_modify(1); 
    bbox(BEGIN, 0.0 , 0.0 , 0.0 , 0.0);
    for(i=0; i<lastselected; i++) {
      int oldbus=0;
      int k = selectedgroup[i].n;
      if(selectedgroup[i].type != WIRE) continue; 
      prepared_hash_wires=0; /* 20181025 */
      prepared_netlist_structs=0;
      prepared_hilight_structs=0;
      oldbus = wire[k].bus;
      if(preserve == 1) {
        set_different_token(&wire[k].prop_ptr, 
               (char *) tclgetvar("retval"), oldprop, 0, 0);
      } else {
        my_strdup(100, &wire[k].prop_ptr,(char *) tclgetvar("retval"));
      }
      bus_ptr = get_tok_value(wire[k].prop_ptr,"bus",0);
      if(!strcmp(bus_ptr, "true")) {
        int ov, y1, y2;
        ov = bus_width > cadhalfdotsize ? bus_width : CADHALFDOTSIZE;
        if(wire[k].y1 < wire[k].y2) { y1 = wire[k].y1-ov; y2 = wire[k].y2+ov; }
        else                        { y1 = wire[k].y1+ov; y2 = wire[k].y2-ov; }
        bbox(ADD, wire[k].x1-ov, y1 , wire[k].x2+ov , y2 );
        wire[k].bus=1;
      } else {
        if(oldbus){ /* 20171201 */
          int ov, y1, y2;
          ov = bus_width> cadhalfdotsize ? bus_width : CADHALFDOTSIZE;
          if(wire[k].y1 < wire[k].y2) { y1 = wire[k].y1-ov; y2 = wire[k].y2+ov; }
          else                        { y1 = wire[k].y1+ov; y2 = wire[k].y2-ov; }
          bbox(ADD, wire[k].x1-ov, y1 , wire[k].x2+ov , y2 );
          wire[k].bus=0;
        }
      }
    }
    bbox(SET , 0.0 , 0.0 , 0.0 , 0.0);
    draw();
    bbox(END , 0.0 , 0.0 , 0.0 , 0.0);
  }
  my_free(727, &oldprop);
}

void edit_arc_property(void)
{
  int old_fill; /* 20180914 */
  double x1, y1, x2, y2;
  int c, i, ii, old_dash, drw = 0;
  char *oldprop = NULL;
  const char *dash;
  int preserve;

  if(arc[selectedgroup[0].col][selectedgroup[0].n].prop_ptr!=NULL) {
    my_strdup(98, &oldprop, arc[selectedgroup[0].col][selectedgroup[0].n].prop_ptr);
    tclsetvar("retval", oldprop);
  } else { /* 20161208 */
    tclsetvar("retval","");
  }
  tcleval("text_line {Input property:} 0 normal");
  preserve = atoi(tclgetvar("preserve_unchanged_attrs"));
  if(strcmp(tclgetvar("rcode"),"") )
  {

   set_modify(1); push_undo(); /* 20150327 */
   for(ii=0; ii<lastselected; ii++) {
     if(selectedgroup[ii].type != ARC) continue;
   
     i = selectedgroup[ii].n;
     c = selectedgroup[ii].col;

     if(preserve == 1) {
        set_different_token(&arc[c][i].prop_ptr, (char *) tclgetvar("retval"), oldprop, 0, 0);

     } else {
        my_strdup(156, &arc[c][i].prop_ptr, (char *) tclgetvar("retval"));
     }
     old_fill = arc[c][i].fill;
     if( !strcmp(get_tok_value(arc[c][i].prop_ptr,"fill",0),"true") )
       arc[c][i].fill =1;
     else 
       arc[c][i].fill =0;
     old_dash = arc[c][i].dash;
     dash = get_tok_value(arc[c][i].prop_ptr,"dash",0);
     if( strcmp(dash, "") ) {
       int d = atoi(dash);
       arc[c][i].dash = d >= 0 ? d : 0;
     } else
       arc[c][i].dash = 0;
  

     if(old_fill != arc[c][i].fill || old_dash != arc[c][i].dash) {
       if(!drw) {
         bbox(BEGIN,0.0,0.0,0.0,0.0);
         drw = 1;
       }
       arc_bbox(arc[c][i].x, arc[c][i].y, arc[c][i].r, 0, 360, &x1,&y1,&x2,&y2);
       bbox(ADD, x1, y1, x2, y2);
     }
   }
   if(drw) {
     bbox(SET , 0.0 , 0.0 , 0.0 , 0.0);
     draw();
     bbox(END , 0.0 , 0.0 , 0.0 , 0.0);
   }
  }
}

void edit_polygon_property(void)
{
  int old_fill; /* 20180914 */
  int k;
  double x1=0., y1=0., x2=0., y2=0.;
  int c, i, ii, old_dash;
  int drw = 0;
  char *oldprop = NULL;
  const char *dash;
  int preserve;

  dbg(1, "edit_property(): input property:\n");
  if(polygon[selectedgroup[0].col][selectedgroup[0].n].prop_ptr!=NULL) {
    my_strdup(112, &oldprop, polygon[selectedgroup[0].col][selectedgroup[0].n].prop_ptr);
    tclsetvar("retval", oldprop);
  } else { /* 20161208 */
    tclsetvar("retval","");
  }
  tcleval("text_line {Input property:} 0 normal");
  preserve = atoi(tclgetvar("preserve_unchanged_attrs"));
  if(strcmp(tclgetvar("rcode"),"") )
  {

   set_modify(1); push_undo(); /* 20150327 */
   for(ii=0; ii<lastselected; ii++) {
     if(selectedgroup[ii].type != POLYGON) continue;
   
     i = selectedgroup[ii].n;
     c = selectedgroup[ii].col;

     if(preserve == 1) {
        set_different_token(&polygon[c][i].prop_ptr, (char *) tclgetvar("retval"), oldprop, 0, 0);

     } else {
        my_strdup(113, &polygon[c][i].prop_ptr, (char *) tclgetvar("retval"));
     }
     old_fill = polygon[c][i].fill;
     old_dash = polygon[c][i].dash;
     if( !strcmp(get_tok_value(polygon[c][i].prop_ptr,"fill",0),"true") )
       polygon[c][i].fill =1;
     else 
       polygon[c][i].fill =0;
     dash = get_tok_value(polygon[c][i].prop_ptr,"dash",0);
     if( strcmp(dash, "") ) {
       int d = atoi(dash);
       polygon[c][i].dash = d >= 0 ? d : 0;
     } else 
       polygon[c][i].dash = 0;
     if(old_fill != polygon[c][i].fill || old_dash != polygon[c][i].dash) {
       if(!drw) {
         bbox(BEGIN,0.0,0.0,0.0,0.0);
         drw = 1;
       }
       for(k=0; k<polygon[c][i].points; k++) {
         if(k==0 || polygon[c][i].x[k] < x1) x1 = polygon[c][i].x[k];
         if(k==0 || polygon[c][i].y[k] < y1) y1 = polygon[c][i].y[k];
         if(k==0 || polygon[c][i].x[k] > x2) x2 = polygon[c][i].x[k];
         if(k==0 || polygon[c][i].y[k] > y2) y2 = polygon[c][i].y[k];
       }
       bbox(ADD, x1, y1, x2, y2);
     }
   }
   if(drw) {
     bbox(SET , 0.0 , 0.0 , 0.0 , 0.0);
     draw();
     bbox(END , 0.0 , 0.0 , 0.0 , 0.0);
   }
  }
}


/* x=0 use text widget   x=1 use vim editor */
void edit_text_property(int x)
{
   #ifdef HAS_CAIRO
   int customfont;
   #endif
   int sel, k, text_changed; 
   int c,l, preserve;
   double xx1,yy1,xx2,yy2;
   double pcx,pcy;      /* pin center 20070317 */
   char property[1024];/* used for float 2 string conv (xscale  and yscale) overflow safe */
   const char *str;
   char *oldprop = NULL;

   dbg(1, "edit_text_property(): entering\n");
   sel = selectedgroup[0].n;
   my_strdup(656, &oldprop, textelement[sel].prop_ptr);
   if(textelement[sel].prop_ptr !=NULL)
      tclsetvar("props",textelement[sel].prop_ptr); 
   else
      tclsetvar("props",""); /* 20171112 */

   tclsetvar("retval",textelement[sel].txt_ptr);
   my_snprintf(property, S(property), "%.16g",textelement[sel].yscale); 
   tclsetvar("vsize",property);
   my_snprintf(property, S(property), "%.16g",textelement[sel].xscale); 
   tclsetvar("hsize",property);
   if(x==0) tcleval("enter_text {text:} normal");
   else if(x==2) tcleval("viewdata $::retval");
   else if(x==1) tcleval("edit_vi_prop {Text:}");
   else {
     fprintf(errfp, "edit_text_property() : unknown parameter x=%d\n",x); exit(EXIT_FAILURE);
   }
   preserve = atoi(tclgetvar("preserve_unchanged_attrs"));
   
   text_changed=0;
   if(x == 0 || x == 1) {
     if( strcmp(textelement[sel].txt_ptr, tclgetvar("retval") ) ) {
       dbg(1, "edit_text_property(): x=%d, text_changed=1\n", x);
       text_changed=1;
     } else {
       dbg(1, "edit_text_property(): x=%d, text_changed=0\n", x);
       text_changed=0;
     }
   }
   if(strcmp(tclgetvar("rcode"),"") )
   {
     dbg(1, "edit_text_property(): rcode !=\"\"\n");
     set_modify(1); push_undo(); /* 20150327 */
     bbox(BEGIN,0.0,0.0,0.0,0.0);
     for(k=0;k<lastselected;k++)
     {
       if(selectedgroup[k].type!=xTEXT) continue;
       sel=selectedgroup[k].n;

       rot = textelement[sel].rot;      /* calculate bbox, some cleanup needed here */
       flip = textelement[sel].flip;
       #ifdef HAS_CAIRO
       customfont = set_text_custom_font(&textelement[sel]);
       #endif
       text_bbox(textelement[sel].txt_ptr, textelement[sel].xscale,
                 textelement[sel].yscale, rot, flip, textelement[sel].hcenter, textelement[sel].vcenter,
                 textelement[sel].x0, textelement[sel].y0,
                 &xx1,&yy1,&xx2,&yy2);
       #ifdef HAS_CAIRO
       if(customfont) cairo_restore(ctx);
       #endif

       bbox(ADD, xx1, yy1, xx2, yy2 );        
    
       dbg(1, "edit_property(): text props: props=%s  text=%s\n",
         tclgetvar("props"),
         tclgetvar("retval") );
       if(text_changed) {
         c = lastrect[PINLAYER];
         for(l=0;l<c;l++) {
           if(!strcmp( (get_tok_value(rect[PINLAYER][l].prop_ptr, "name",0)),
                        textelement[sel].txt_ptr) ) {
             #ifdef HAS_CAIRO
             customfont = set_text_custom_font(&textelement[sel]);
             #endif
             text_bbox(textelement[sel].txt_ptr, textelement[sel].xscale,
             textelement[sel].yscale, rot, flip, textelement[sel].hcenter, textelement[sel].vcenter,
             textelement[sel].x0, textelement[sel].y0,
             &xx1,&yy1,&xx2,&yy2);
             #ifdef HAS_CAIRO
             if(customfont) cairo_restore(ctx);
             #endif

             pcx = (rect[PINLAYER][l].x1+rect[PINLAYER][l].x2)/2.0;
             pcy = (rect[PINLAYER][l].y1+rect[PINLAYER][l].y2)/2.0;

             if(
                 /* 20171206 20171221 */
                 (fabs( (yy1+yy2)/2 - pcy) < cadgrid/2 && 
                 (fabs(xx1 - pcx) < cadgrid*3 || fabs(xx2 - pcx) < cadgrid*3) )
                 || 
                 (fabs( (xx1+xx2)/2 - pcx) < cadgrid/2 && 
                 (fabs(yy1 - pcy) < cadgrid*3 || fabs(yy2 - pcy) < cadgrid*3) )
             ) {
               if(x==0)  /* 20080804 */
                 my_strdup(71, &rect[PINLAYER][l].prop_ptr, 
                   subst_token(rect[PINLAYER][l].prop_ptr, "name", 
                   (char *) tclgetvar("retval")) );
               else
                 my_strdup(72, &rect[PINLAYER][l].prop_ptr, 
                   subst_token(rect[PINLAYER][l].prop_ptr, "name", 
                   (char *) tclgetvar("retval")) );
             }
           }
         } 
         my_strdup(74, &textelement[sel].txt_ptr, (char *) tclgetvar("retval"));
       }
       if(x==0) {
         if(preserve) 
          set_different_token(&textelement[sel].prop_ptr, (char *) tclgetvar("props"), oldprop, 0, 0);
         else 
           my_strdup(75, &textelement[sel].prop_ptr,(char *) tclgetvar("props"));
         my_strdup(76, &textelement[sel].font, get_tok_value(textelement[sel].prop_ptr, "font", 0));/*20171206 */

         str = get_tok_value(textelement[sel].prop_ptr, "hcenter", 0);
         textelement[sel].hcenter = strcmp(str, "true")  ? 0 : 1;
         str = get_tok_value(textelement[sel].prop_ptr, "vcenter", 0);
         textelement[sel].vcenter = strcmp(str, "true")  ? 0 : 1;

         str = get_tok_value(textelement[sel].prop_ptr, "layer", 0); /* 20171206 */
         if(str[0]) textelement[sel].layer = atoi(str);
         else textelement[sel].layer=-1;


         textelement[sel].flags = 0;
         str = get_tok_value(textelement[sel].prop_ptr, "slant", 0);
         textelement[sel].flags |= strcmp(str, "oblique")  ? 0 : TEXT_OBLIQUE;
         textelement[sel].flags |= strcmp(str, "italic")  ? 0 : TEXT_ITALIC;
         str = get_tok_value(textelement[sel].prop_ptr, "weight", 0);
         textelement[sel].flags |= strcmp(str, "bold")  ? 0 : TEXT_BOLD;

         textelement[sel].xscale=atof(tclgetvar("hsize"));
         textelement[sel].yscale=atof(tclgetvar("vsize"));
       }
    
                                /* calculate bbox, some cleanup needed here */
       #ifdef HAS_CAIRO
       customfont = set_text_custom_font(&textelement[sel]);
       #endif
       text_bbox(textelement[sel].txt_ptr, textelement[sel].xscale,
                 textelement[sel].yscale, rot, flip, textelement[sel].hcenter, textelement[sel].vcenter,
                 textelement[sel].x0, textelement[sel].y0,
                 &xx1,&yy1,&xx2,&yy2);
       #ifdef HAS_CAIRO
       if(customfont) cairo_restore(ctx);
       #endif

       bbox(ADD, xx1, yy1, xx2, yy2 );        
    
     }
     bbox(SET,0.0,0.0,0.0,0.0);
     draw();
     bbox(END,0.0,0.0,0.0,0.0);
   }
   my_free(890, &oldprop);
}

static char *old_prop=NULL;
static int i=-1;
static int netlist_commands;

/* x=0 use text widget   x=1 use vim editor */
void edit_symbol_property(int x)
{
   char *result=NULL;

   i=selectedgroup[0].n;
   netlist_commands = 0;
   if ((inst_ptr[i].ptr + instdef)->type!=NULL)
     netlist_commands =  !strcmp( (inst_ptr[i].ptr+instdef)->type, "netlist_commands");

   if(inst_ptr[i].prop_ptr!=NULL) {
     if(netlist_commands && x==1) {
       tclsetvar("retval",get_tok_value( inst_ptr[i].prop_ptr,"value",0));
     } else {
       tclsetvar("retval",inst_ptr[i].prop_ptr);
     }
   }
   else {
     tclsetvar("retval","");
   }
   my_strdup(91, &old_prop, inst_ptr[i].prop_ptr);
   tclsetvar("symbol",inst_ptr[i].name);

   if(x==0) {
     tcleval("edit_prop {Input property:}");
     my_strdup(77, &result, tclresult());
   }
   else {
     /* edit_vi_netlist_prop will replace \" with " before editing,
        replace back " with \" when done and wrap the resulting text with quotes
        ("text") when done */
     if(netlist_commands && x==1)    tcleval("edit_vi_netlist_prop {Input property:}");
     else if(x==1)    tcleval("edit_vi_prop {Input property:}");
     else if(x==2)    tcleval("viewdata $::retval");
     my_strdup(78, &result, tclresult());
   }
   dbg(1, "edit_symbol_property(): before update_symbol, modified=%d\n", modified);
   update_symbol(result, x);
   my_free(728, &result);
   dbg(1, "edit_symbol_property(): done update_symbol, modified=%d\n", modified);
   i=-1; /* 20160423 */
}

/* x=0 use text widget   x=1 use vim editor */
void update_symbol(const char *result, int x)
{
  int k, sym_number;
  int no_change_props=0;
  int only_different=0;
  int copy_cell=0; /* 20150911 */
  int prefix=0;
  char *name = NULL, *ptr = NULL, *new_prop = NULL;
  char symbol[PATH_MAX];
  char *type;
  const char *new_name;
  int cond, allow_change_name;
  int pushed=0; /* 20150327 */

  dbg(1, "update_symbol(): entering\n");
  i=selectedgroup[0].n; /* 20110413 */
  if(!result) {
   dbg(1, "update_symbol(): edit symbol prop aborted\n");
   return;
  }

  /* create new_prop updated attribute string */
  if(netlist_commands && x==1) {
    my_strdup(79,  &new_prop,
      subst_token(old_prop, "value", (char *) tclgetvar("retval") )
    );
    dbg(1, "update_symbol(): new_prop=%s\n", new_prop);
    dbg(1, "update_symbol(): tcl retval==%s\n", tclgetvar("retval"));
  }
  else {
    my_strdup(80, &new_prop, (char *) tclgetvar("retval"));
    dbg(1, "update_symbol(): new_prop=%s\n", new_prop);
  }

  my_strncpy(symbol, (char *) tclgetvar("symbol") , S(symbol));
  dbg(1, "update_symbol(): symbol=%s\n", symbol);
  no_change_props=atoi(tclgetvar("no_change_attrs") );
  only_different=atoi(tclgetvar("preserve_unchanged_attrs") );
  copy_cell=atoi(tclgetvar("user_wants_copy_cell") ); /* 20150911 */

  bbox(BEGIN,0.0,0.0,0.0,0.0);
  if(show_pin_net_names) {
    prepare_netlist_structs(0);
    find_inst_hash_clear();
    for(k = 0;  k < (inst_ptr[i].ptr + instdef)->rects[PINLAYER]; k++) {
      if( inst_ptr[i].node && inst_ptr[i].node[k]) {
         find_inst_to_be_redrawn(inst_ptr[i].node[k]);
      }
    }
  }

  /* 20191227 necessary? --> Yes since a symbol copy has already been done 
     in edit_symbol_property() -> tcl edit_prop, this ensures new symbol is loaded from disk.
     if for some reason a symbol with matching name is loaded in xschem this
     may be out of sync wrt disk version */
  if(copy_cell) {
   remove_symbols();
   link_symbols_to_instances();
  }
  
  /* symbol reference changed? --> sym_number >=0, set prefix to 1st char
     to use for inst name (from symbol template) */
  prefix=0;
  sym_number = -1;
  if(strcmp(symbol, inst_ptr[i].name)) {
    set_modify(1);
    prepared_hash_instances=0;
    prepared_netlist_structs=0;
    prepared_hilight_structs=0;
    sym_number=match_symbol(symbol); /* check if exist */
    if(sym_number>=0) {
      prefix=(get_tok_value((instdef+sym_number)->templ, "name",0))[0]; /* get new symbol prefix  */
    }
  }

  /* instance name prefix (1st char) changed? --> allow_change_name=1 */
  allow_change_name = 0;
  if(new_prop) {
    my_strdup(88, &name, get_tok_value(inst_ptr[i].prop_ptr, "name", 0));
    new_name = get_tok_value(new_prop, "name", 0);
    if(!name || new_name[0] != name[0]) allow_change_name = 1;
  }
  for(k=0;k<lastselected;k++) {
   dbg(1, "update_symbol(): for k loop: k=%d\n", k);
   if(selectedgroup[k].type!=ELEMENT) continue;
   i=selectedgroup[k].n;

   /* 20171220 calculate bbox before changes to correctly redraw areas */
   /* must be recalculated as cairo text extents vary with zoom factor. */
   symbol_bbox(i, &inst_ptr[i].x1, &inst_ptr[i].y1, &inst_ptr[i].x2, &inst_ptr[i].y2);

   if(sym_number>=0) /* changing symbol ! */
   {
     if(!pushed) { push_undo(); pushed=1;}
     delete_inst_node(i); /* 20180208 fix crashing bug: delete node info if changing symbol */
                          /* if number of pins is different we must delete these data *before* */
                          /* changing ysmbol, otherwise i might end up deleting non allocated data. */
     my_strdup(82, &inst_ptr[i].name, rel_sym_path(symbol));
     if(event_reporting) {
       char n1[PATH_MAX];
       char n2[PATH_MAX];
       printf("xschem replace_symbol instance %s %s\n",
           escape_chars(n1, inst_ptr[i].instname, PATH_MAX),
           escape_chars(n2, symbol, PATH_MAX)
       );
       fflush(stdout);
     }
     inst_ptr[i].ptr=sym_number; /* update instance to point to new symbol */
   }
   bbox(ADD, inst_ptr[i].x1, inst_ptr[i].y1, inst_ptr[i].x2, inst_ptr[i].y2);

   /* update property string from tcl dialog */
   if(!no_change_props)
   {
     dbg(1, "update_symbol(): no_change_props=%d\n", no_change_props);
     if(only_different) {
       char * ss=NULL;
       my_strdup(119, &ss, inst_ptr[i].prop_ptr);
       if( set_different_token(&ss, new_prop, old_prop, 0, 0) ) {
         if(!pushed) { push_undo(); pushed=1;}
         my_strdup(111, &inst_ptr[i].prop_ptr, ss);
         set_modify(1);
         prepared_hash_instances=0; /* 20171224 */
         prepared_netlist_structs=0;
         prepared_hilight_structs=0;
       }
       my_free(729, &ss);
     }
     else {
       if(new_prop) {  /* 20111205 */
         if(!inst_ptr[i].prop_ptr || strcmp(inst_ptr[i].prop_ptr, new_prop)) {
           dbg(1, "update_symbol(): changing prop: |%s| -> |%s|\n", inst_ptr[i].prop_ptr, new_prop);
           if(!pushed) { push_undo(); pushed=1;}
           my_strdup(84, &inst_ptr[i].prop_ptr, new_prop);
           set_modify(1);
           prepared_hash_instances=0; /* 20171224 */
           prepared_netlist_structs=0;
           prepared_hilight_structs=0;
         }
       }  else {  /* 20111205 */
         if(!pushed) { push_undo(); pushed=1;}
         my_strdup(86, &inst_ptr[i].prop_ptr, "");
         set_modify(1);
         prepared_hash_instances=0; /* 20171224 */
         prepared_netlist_structs=0;
         prepared_hilight_structs=0;
       }
     }
   }

   /* if symbol changed ensure instance name (with new prefix char) is unique */
   my_strdup(152, &name, get_tok_value(inst_ptr[i].prop_ptr, "name", 0));
   if(name && name[0] ) {  
     dbg(1, "update_symbol(): prefix!='\\0', name=%s\n", name);
     new_name = get_tok_value(inst_ptr[i].prop_ptr, "name", 0);
     if(allow_change_name || (lastselected == 1) ) my_strdup(153, &name, new_name);
     /* 20110325 only modify prefix if prefix not NUL */
     if(prefix) name[0]=prefix; /* change prefix if changing symbol type; */
     dbg(1, "update_symbol(): name=%s, inst_ptr[i].prop_ptr=%s\n", name, inst_ptr[i].prop_ptr);
     my_strdup(89, &ptr,subst_token(inst_ptr[i].prop_ptr, "name", name) );
                    /* set name of current inst */
 
     if(!pushed) { push_undo(); pushed=1;}
     if(!k) hash_all_names(i);
     new_prop_string(i, ptr, k, disable_unique_names); /* set new prop_ptr */
 
     type=instdef[inst_ptr[i].ptr].type; /* 20150409 */
     cond= !type || (strcmp(type,"label") && strcmp(type,"ipin") && strcmp(type,"show_label") &&
           strcmp(type,"opin") &&  strcmp(type,"iopin"));
     if(cond) inst_ptr[i].flags|=2; /* bit 1: flag for different textlayer for pin/labels */
     else inst_ptr[i].flags &=~2;
   }

   if(event_reporting) {
     char *ss=NULL;
     my_strdup(120, &ss, inst_ptr[i].prop_ptr);
     set_different_token(&ss, new_prop, old_prop, ELEMENT, i);
     my_free(730, &ss);
   }
   my_strdup2(90, &inst_ptr[i].instname, get_tok_value(inst_ptr[i].prop_ptr, "name",0)); /* 20150409 */
  }  /* end for(k=0;k<lastselected;k++) */

   /* new symbol bbox after prop changes (may change due to text length) */
  if(modified) {
    prepared_hash_instances=0;
    prepared_netlist_structs=0;
    prepared_hilight_structs=0;
    for(k=0;k<lastselected;k++) {
      if(selectedgroup[k].type!=ELEMENT) continue;
      i=selectedgroup[k].n;
      symbol_bbox(i, &inst_ptr[i].x1, &inst_ptr[i].y1, &inst_ptr[i].x2, &inst_ptr[i].y2);
      bbox(ADD, inst_ptr[i].x1, inst_ptr[i].y1, inst_ptr[i].x2, inst_ptr[i].y2);
    }
  }


  if(show_pin_net_names) {
    prepare_netlist_structs(0);
    find_inst_hash_clear();
    for(k = 0;  k < (inst_ptr[i].ptr + instdef)->rects[PINLAYER]; k++) {
      if( inst_ptr[i].node && inst_ptr[i].node[k]) {
         find_inst_to_be_redrawn(inst_ptr[i].node[k]);
      }
    }
  }

  /* redraw symbol with new props */
  bbox(SET,0.0,0.0,0.0,0.0);
  dbg(1, "update_symbol(): redrawing inst_ptr.txtprop string\n");
  draw();
  bbox(END,0.0,0.0,0.0,0.0);
  my_free(731, &name);
  my_free(732, &ptr);
  my_free(733, &new_prop);
  my_free(734, &old_prop);
}

void change_elem_order(void)
{
   Instance tmpinst;
   Box tmpbox;
   Wire tmpwire;
   char tmp_txt[50]; /* overflow safe */
   int c, new_n;

    rebuild_selected_array();
    if(lastselected==1)
    {
     my_snprintf(tmp_txt, S(tmp_txt), "%d",selectedgroup[0].n);
     tclsetvar("retval",tmp_txt);
     tcleval("text_line {Object Sequence number} 0");
     if(strcmp(tclgetvar("rcode"),"") )
     {
      push_undo(); /* 20150327 */
      set_modify(1);
      prepared_hash_instances=0; /* 20171224 */
      prepared_netlist_structs=0;
      prepared_hilight_structs=0;
     }
     sscanf(tclgetvar("retval"), "%d",&new_n);

     if(selectedgroup[0].type==ELEMENT)
     {
      if(new_n>=lastinst) new_n=lastinst-1;
      tmpinst=inst_ptr[new_n];
      inst_ptr[new_n]=inst_ptr[selectedgroup[0].n];
      inst_ptr[selectedgroup[0].n]=tmpinst;
      dbg(1, "change_elem_order(): selected element %d\n", selectedgroup[0].n);
     }
     else if(selectedgroup[0].type==xRECT)
     {
      c=selectedgroup[0].col;
      if(new_n>=lastrect[c]) new_n=lastrect[c]-1;
      tmpbox=rect[c][new_n];
      rect[c][new_n]=rect[c][selectedgroup[0].n];
      rect[c][selectedgroup[0].n]=tmpbox;
      dbg(1, "change_elem_order(): selected element %d\n", selectedgroup[0].n);
     }
     else if(selectedgroup[0].type==WIRE)
     {
      if(new_n>=lastwire) new_n=lastwire-1;
      tmpwire=wire[new_n];
      wire[new_n]=wire[selectedgroup[0].n];
      wire[selectedgroup[0].n]=tmpwire;
      dbg(1, "change_elem_order(): selected element %d\n", selectedgroup[0].n);
     }

    }
}

/* x=0 use tcl text widget  x=1 use vim editor  x=2 only view data */
void edit_property(int x)
{
 int j;

 if(!has_x) return;
 rebuild_selected_array(); /* from the .sel field in objects build */
 if(lastselected==0 )      /* the array of selected objs */
 {
   char *old_prop = NULL;
   char *new_prop = NULL;

   if(netlist_type==CAD_SYMBOL_ATTRS) {
    if(schsymbolprop!=NULL)    /*09112003 */
      tclsetvar("retval",schsymbolprop);
    else
      tclsetvar("retval","");
   }
   else if(netlist_type==CAD_VHDL_NETLIST) {
    if(schvhdlprop!=NULL)    /*09112003 */
      tclsetvar("retval",schvhdlprop);
    else
      tclsetvar("retval","");
   }
   else if(netlist_type==CAD_VERILOG_NETLIST) {
    if(schverilogprop!=NULL)    /*09112003 */
      tclsetvar("retval",schverilogprop);
    else
      tclsetvar("retval","");
   }
   else if(netlist_type==CAD_SPICE_NETLIST) { /* 20100217 */
    if(schprop!=NULL) 
      tclsetvar("retval",schprop);
    else
      tclsetvar("retval","");
   }
   else if(netlist_type==CAD_TEDAX_NETLIST) { /* 20100217 */
    if(schtedaxprop!=NULL) 
      tclsetvar("retval",schtedaxprop);
    else
      tclsetvar("retval","");
   }
   my_strdup(660, &old_prop, tclgetvar("retval"));

   if(x==0)         tcleval("text_line {Global schematic property:} 0");          
   else if(x==1) {
      dbg(1, "edit_property(): executing edit_vi_prop\n");
      tcleval("edit_vi_prop {Global schematic property:}");
   }
   else if(x==2)    tcleval("viewdata $::retval");
   dbg(1, "edit_property(): done executing edit_vi_prop, result=%s\n",tclresult());
   dbg(1, "edit_property(): rcode=%s\n",tclgetvar("rcode") );

   my_strdup(650, &new_prop, (char *) tclgetvar("retval"));
   tclsetvar("retval", new_prop);
   my_free(892, &old_prop);
   my_free(893, &new_prop);


   if(strcmp(tclgetvar("rcode"),"") )
   {
     if(netlist_type==CAD_SYMBOL_ATTRS && /* 20120228 check if schprop NULL */
        (!schsymbolprop || strcmp(schsymbolprop, tclgetvar("retval") ) ) ) { /* 20120209 */
        set_modify(1); push_undo(); /* 20150327 */
        my_strdup(422, &schsymbolprop, (char *) tclgetvar("retval")); /*09112003  */

     } else if(netlist_type==CAD_VERILOG_NETLIST && /* 20120228 check if schverilogprop NULL */
        (!schverilogprop || strcmp(schverilogprop, tclgetvar("retval") ) ) ) { /* 20120209 */
        set_modify(1); push_undo(); /* 20150327 */
        my_strdup(94, &schverilogprop, (char *) tclgetvar("retval")); /*09112003 */
    
     } else if(netlist_type==CAD_SPICE_NETLIST && /* 20120228 check if schprop NULL */
        (!schprop || strcmp(schprop, tclgetvar("retval") ) ) ) { /* 20120209 */
        set_modify(1); push_undo(); /* 20150327 */
        my_strdup(95, &schprop, (char *) tclgetvar("retval")); /*09112003  */

     } else if(netlist_type==CAD_TEDAX_NETLIST && /* 20120228 check if schprop NULL */
        (!schtedaxprop || strcmp(schtedaxprop, tclgetvar("retval") ) ) ) { /* 20120209 */
        set_modify(1); push_undo(); /* 20150327 */
        my_strdup(96, &schtedaxprop, (char *) tclgetvar("retval")); /*09112003  */

     } else if(netlist_type==CAD_VHDL_NETLIST && /* 20120228 check if schvhdlprop NULL */
        (!schvhdlprop || strcmp(schvhdlprop, tclgetvar("retval") ) ) ) { /* netlist_type==CAD_VHDL_NETLIST */
        set_modify(1); push_undo(); /* 20150327 */
        my_strdup(97, &schvhdlprop, (char *) tclgetvar("retval"));
     }
   }

   /* update the bounding box of vhdl "architecture" instances that embed */
   /* the schvhdlprop string. 04102001 */
   for(j=0;j<lastinst;j++)
   {
    if( inst_ptr[j].ptr !=-1 && 
        (inst_ptr[j].ptr+instdef)->type &&
        !strcmp( (inst_ptr[j].ptr+instdef)->type, "architecture") ) /* 20150409 */
    {
      dbg(1, "edit_property(): updating vhdl architecture\n");
      symbol_bbox(j, &inst_ptr[j].x1, &inst_ptr[j].y1,
                        &inst_ptr[j].x2, &inst_ptr[j].y2);
    } 
   } /* end for(j */
   return;
 }

 switch(selectedgroup[0].type)
 {
  case ELEMENT:
   edit_symbol_property(x);
   while( x == 0 && tclgetvar("edit_symbol_prop_new_sel")[0] == '1' ) {
     unselect_all();
     select_object(mousex, mousey, SELECTED, 0);
     rebuild_selected_array();
     if(lastselected && selectedgroup[0].type ==ELEMENT) {
       edit_symbol_property(0);
     } else {
       break;
     }
   }
   tclsetvar("edit_symbol_prop_new_sel", "");
   break;
  case ARC:
   edit_arc_property();
   break;
  case xRECT:
   edit_rect_property();
   break;
  case WIRE:
   edit_wire_property();
   break;
  case POLYGON: /* 20171115 */
   edit_polygon_property();
   break;
  case LINE:
   edit_line_property();
   break;
  case xTEXT:
   edit_text_property(x);
   break;             
 }

}


