#!/usr/bin/gawk -f



FNR == 1 {
    if (_filename_ != "")
        endfile(_filename_)
    _filename_ = FILENAME
    #print "processing: " FILENAME >"/dev/stderr"
    beginfile(FILENAME)
}

END  { endfile(_filename_) }

###### begin user code ########################

BEGIN{
}


#  T {@#3:net_name} 25 1.25 0 0 0.15 0.15 {layer=2}
/^T {.*net_name.*layer=12/{
  found = 1
  sub(/layer=12/, "layer=15")
}


function replace_pattern(old, new)
{
  if($0 ~ old) {
    gsub(old, new)
    found = 1
  }
}


###### end  user code  ########################


{
 __a[__lines++] = $0 
}

function beginfile(f)
{
 __lines=0
 found=0
}

function endfile(f,   i)
{
 if(found)  {
   print "patching: " f >"/dev/stderr"
   for(i=0;i<__lines;i++)
   {
    print __a[i] > f
   }
   close(f)
 }
} 

