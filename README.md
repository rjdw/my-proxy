# my-proxy    
Project coded for CS024 of California Institute of Technology   
Under the Guidance of Professor Adam Blank      

## description
The project consists of the multi-threaded web proxy with LRU caching capabilities. The LRU cache is implemented with a custom hash table made for concurrent threading (this was done using the simplest method, read-write locks). This means that if concurrent threads were accessing the LRU cache to update the timestamp of different sites, the thread locks may interfere with the exact accuracy of the caching. In this manner, the LRU cache is somewhat inexact (though who really cares).      
I guess one could use an atomic hash table or some sort of high level thread safe structure for the cache to eliminate caching inaccuracy. Not really worth the trouble.    

## use
Just run the Makefile and the executable.      
The Makefile requires clang-with-asan so download that if you want to; otherwise, compile with any C compiler.     
Makefile commands:     
`make clean`      
`make run`     
Then run the outputed executable to start the web proxy.


