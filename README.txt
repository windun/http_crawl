***************************************************************************************************************
**************************************************************************************************** * *** * *
** 											****** *** * **** *
** DASON ADAMOS 									******* * * *
** CSC 718 				HTTP_CRAWL - README 				* *** * *
** 09DEC2014 										* * *** *
** 											****
******************************************************************************************* *
******************************************************************************************

. . . . . . . . . . . . . . . . INTRODUCTION. . . . . . . . . . . . . . . .

In depth documentation: http://students.dsu.edu/DSU/ddadamos/webcrawler/

git repository: https://github.com/laterDays/http_crawl

The files included in this project are the files necessary to build the
simple webcrawler. I have made all steps simple by utilizing one line 
scripts.


0. preparation: unzip files into a directory. Create a "data" folder:

http_crawl/Parser.h
http_crawl/README.txt
http_crawl/clear
http_crawl/comp
http_crawl/crawl.cpp
http_crawl/crawl.sh
http_crawl/crawl_mem
http_crawl/robots.h
http_crawl/data/     <- make this

1. install libcurl

On ubuntu: 
user@host-$ sudo apt-get install libcurl-dev

On fedora:
user@host-$ sudo yum install libcurl-devel


2. compilation

user@host-$ ./comp


3. execution

user@host-$ ./crawl [url] [#pages to download]
user@host-$ ./crawl google.com 100 


4. results

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



