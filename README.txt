*********************************************************************************
************************************************************************* * *** * *
** 								****** *** * **** *
** DASON ADAMOS 						******* * * *
** CSC 718 		HTTP_CRAWL - README 			* *** * *
** 09DEC2014 							* * *** *
** 								****
******************************************************************** *
*********************************************************************

. . . . . . . . . . . . . . . . INTRODUCTION. . . . . . . . . . . . . . . .

In depth documentation: http://students.dsu.edu/DSU/ddadamos/webcrawler/

git repository: https://github.com/laterDays/http_crawl

The files included in this project are the files necessary to build the
simple webcrawler. At this time, the webcrawler is programmed to run
alongside Neo4j. Please install Neo4j before running this webcrawler.




0. preparation: unzip files into a directory. Create a "data" folder:


http_crawl/clear			data clear script
http_crawl/comp				compiling script
http_crawl/crawl.cpp		
http_crawl/crawl.sh
http_crawl/crawl_mem			valgrid test run
http_crawl/robots.h
http_crawl/data/     			* make this directory
http_crawl/Neo4jConn.h		Neo4j Connections handled here
http_crawl/Parser.h		HTML parser and JSON builder
http_crawl/robots.h
http_crawl/README.txt




1a. install libcurl

On ubuntu: 
user@host-$ apt-get install libcurl4-gnutls-dev

On fedora:
user@host-$ sudo yum install libcurl-devel



1b. install jsoncpp - this library is used form JSON processing

user@host-$ studo apt-get instsall cmake
user@host-$ git clone https://github.com/open-source-parsers/jsoncpp
user@host-$ cd jsoncpp
user@host-$ mkdir -p build/debug
user@host-$ cd build/debug
user@host-$ cmake -DCMAKE_BUILD_TYPE=debug -DJSONCPP_LIB_BUILD_SHARED=OFF -G "Unix Makefiles" ../..
user@host-$ make
user@host-$ sudo mkdir /usr/lib/json
user@host-$ sudo cp build/debug/lib/libjsoncpp.a /usr/lib/json/
user@host-$ sudo cp include/json /usr/include/

2. compilation

user@host-$ ./comp





3. execution

a. Data gathering

//user@host-$ ./crawl [url] [#pages to download]
user@host-$ ./crawl google.com 100 

b. Querying

// user@host-$ ./crawl -q ["query"] [row,graph,row/graph]
user@host-$ ./crawl -q "MATCH (a) RETURN a" "row"

c. "Pieced Query"

// user@host-$ ./crawl -pq [nodes/edges] [id/label/properties] [property] [value]

search for node whose id=1
user@host-$ ./crawl -pq nodes id '' 1	

search for edge whose id=1
user@host-$ ./crawl -pq edges id '' 1	

search for nodes with label URL
user@host-$ ./crawl -pq nodes label '' URL

search for nodes that have an href property with the letter "h" in,
it, for example node.href="homepage.com"
user@host-$ ./crawl -pq nodes properties href .*h.*



4. results

The queries that are performed run on Neo4j. However, during the web
crawl, the following files are generated:

http_crawl/data/*_head.txt	// HTTP header
http_crawl/data/*_body.txt	// HTML text
http_crawl/data/*_tags.txt	// Parser output

An example from one run "0_tags.txt":

0,0  base
1,0  HTML
2,1  HEAD
3,2  meta
     http-equiv = content-type
     content = text/html;charset=utf-8
     content (1) = "
..."
4,3  TITLE
     content (9) = "301 Moved..."

The 4th line indicates that a "meta" tag was found. That tag was assigned
an id of 3 and it is nested within tag 2 (HEAD). The lines following indicate
that meta had attributes http-equiv with a value "content-type" and content with
a value of "text/html;charset=utf-8". Reasoning about this would lead us to think 
the meta tag originally looked something like this:

<HEAD>
     <meta http-equiv="content-type" content="text/html;charset=utf-8">
     ...
</HEAD>




5. clear data

user@host-$ ./clear






