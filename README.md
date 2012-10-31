
# An example to tell you how to write C++ as script

rule 1: don't design class

rule 2: compiler will know what is the best code
aka: rule 2: write down the idea and let the compiler optimize

rule 3: use boost


# real readme for googleproxy ==

why I write this:
	force the browser to use https://


add 127.0.0.1 www.google.com to  /etc/hosts

run googleproxy 

then you will default to use https . enjoy :)


build dep:
	  if you checkout on git, you'll need automake/autoconf/libtool/boost-m4
	  
	  run libtoolize && autoreconf 
	  
	  now start build
	  
build dep: need boost. that's all.
	  
	  ./configure && make && make install
	  
