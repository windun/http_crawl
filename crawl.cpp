/*
	Compile with: g++ crawl.cpp -o crawl -lcurl -std=c++11

*/
#include <cstring>			// memcpy () - aggregating data received by CURLOPT_WRITEFUNCTION (curl.h)
#include <unordered_map>	// for URLS - data structure to track all urls, their source and destinations
#include <unordered_set>	// for URLS
#include <curl/curl.h>		// http gathering
#include <regex.h> 			// link recognition (vs. file)
#include "Parser.h"

#define REGEX_PATTERN "[a-zA-Z0-9./]*(.bmp|.gif|.jpg|.pdf|.png)"	// link recognition - regex.h
#define DATA_DIR "data/"		// fwrite () into this directory
#define INDENT_STEP 5			// print_column () - indent columns
#define COLUMN_WIDTH 40			// print_column () - width of block to print
#define MAX_BUF_SIZE 1000000	// for UrlData buffer (stores html data)
#define MSG_BUF_SIZE 4096		// link recognition (regex.h) error
#define MAX_CURL_DEPTH 2		// limit depth of data gathering

std::unordered_map<std::string, std::unordered_set<std::string>*> URLS;
struct UrlData
{
	std::string source;
	FILE * file;
	char buffer [MAX_BUF_SIZE];
	int buf_size = 0;
	int level;
};
void print_indent (int indent)
{
	for (int i = 0; i < indent; i++)
	{
		fprintf(stdout, " ");
	}
}
void print_column (const char * str, int width)
{
	for (int i = 0; i < width && str[i] != '\0'; i++)
	{
		fprintf(stdout, "%c", str[i]);
	}
	fprintf(stdout, "\n");
}

void add_url (std::string source, std::string target)
{
	std::unordered_set<std::string> *target_urls = URLS.at(source);
	target_urls->insert(target);
}

bool is_link (char * str, int level)
{
	int indent = level * INDENT_STEP;
	std::string url;
	regex_t regex;
	int reti;
	char buf [MSG_BUF_SIZE];

	// match image files, is_link() should return false for
	// thee matches, so we will negate the result
	reti = regcomp(&regex, REGEX_PATTERN, REG_EXTENDED);
	if (reti){fprintf(stderr, "Could not compile regex\n"); exit(1);}

	reti = regexec(&regex, str, 0, NULL, 0);
	if(!reti)
	{
		url = "[+] "; url += str;
		print_indent(level * INDENT_STEP);print_column(url.c_str(), COLUMN_WIDTH);
		return true;
	}
	else if (reti == REG_NOMATCH)
	{	
		url = "[ ] "; url += str;
		print_indent(level * INDENT_STEP);print_column(url.c_str(), COLUMN_WIDTH);
		return false;
	}
	else
	{
		regerror(reti, &regex, buf, sizeof(buf));
		fprintf(stderr, "Regex match failed: %s\n", buf);
		exit(1);
	}
}
std::string toString (char *c_str)
{
	std::string str = "";
	// Convert only the symbols, letters, and numbers
	// to strings. I had a hard time with an ASCII 13
	// (carriage regurn) being copied into the string.
	for (int i = 0; c_str[i] >= 33 && c_str[i] <= 126; i++)
	{
		str += c_str[i];	
	} 
	return str;
}
std::string toString (const char *c_str)
{
	std::string str = "";
	for (int i = 0; c_str[i] != '\0'; i++)
	{
		str += c_str[i];	
	} 
	return str;
}

static size_t write_header (void *ptr, size_t size, size_t nmemb, void *data)
{
	int written = fwrite(ptr, size, nmemb, ((struct UrlData *)data)->file);
	return written;
}

// called during curl_easy_perform ()
// ptr = data from internet
// size = #packets ?
// nmemb = amount data per packet
// data = my data
static size_t write_body (void *ptr, size_t size, size_t nmemb, void *data)
{
	UrlData *url_data = (struct UrlData *)data;
	char *mem_buf = url_data->buffer;
	int written = fwrite(ptr, size, nmemb, url_data->file);

	int mem_size = size * nmemb;
	memcpy(&(mem_buf[url_data->buf_size]), (char *)ptr, mem_size);
	url_data->buf_size += mem_size;
	mem_buf[url_data->buf_size] = 0;	// null terminate 1 index past end

	return written;
}


int mCurl (const char* source_url, int level)
{
	struct UrlData header_data;
	struct UrlData body_data;

	CURL *curl_handle;
	CURLcode res;
	std::string headerFilename = DATA_DIR + std::to_string(level) + "_" + std::string(source_url) + "_head.out";
	std::string bodyFilename = DATA_DIR + std::to_string(level) + "_" + std::string(source_url) + "_body.out";
	
	std::list<std::string> *new_URLS;
	std::unordered_set<std::string> *target_URLS;

	try
	{
		URLS.at(source_url);
	}
	catch (std::exception& e)
	{
		target_URLS = new std::unordered_set<std::string> ();
		URLS.insert({{source_url, target_URLS}});
	}

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if(curl_handle)
	{

		/////////////////////////////////////////////
		//	SET UP CURL
		//std::string heading = "[" + level + "]-> " + source_url + "\n";
		std::string heading = "[" + std::to_string(level) + "]-> " + source_url;
		print_indent(level * INDENT_STEP); print_column (heading.c_str(), COLUMN_WIDTH);
		curl_easy_setopt(curl_handle, CURLOPT_URL, source_url);
		//curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);	
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_body);

		/////////////////////////////////////////////
		//	SET UP FILES
		header_data.file = fopen(headerFilename.c_str(), "wb");
		if (header_data.file == NULL)
		{
			curl_easy_cleanup(curl_handle);
			return -1;
		}
		body_data.file = fopen(bodyFilename.c_str(), "wb");
		if (body_data.file == NULL)
		{
			curl_easy_cleanup(curl_handle);
			return -1;
		}
		header_data.source = toString(source_url);
		header_data.level = level;
		body_data.source = toString(source_url);
		body_data.level = level;

		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &header_data);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body_data);


 		/////////////////////////////////////////////
		//	PERFORM THE CURL
		res = curl_easy_perform(curl_handle);


		
		if (res != CURLE_OK)
		{
			print_indent(level * INDENT_STEP); fprintf(stdout, "[x]-> %s\n", curl_easy_strerror(res));
			remove(headerFilename.c_str());
			remove(bodyFilename.c_str());
		}
		curl_easy_cleanup(curl_handle);
		
		Parser Parser_((char *)body_data.buffer);
		Parser_.set_debug(false);
		Parser_.process();

		new_URLS = Parser_.get_attribute_values("href");

		for (std::list<std::string>::iterator it=new_URLS->begin(); it != new_URLS->end(); it++)
		{
			target_URLS->insert(*it);
		}

		std::string footer = "[+] " + std::to_string(URLS.at(source_url)->size()) + " urls found.";
		print_indent(level * INDENT_STEP); print_column (footer.c_str(), COLUMN_WIDTH);
		int n = 0; 
		std::string previous;
		for (std::unordered_set<std::string>::iterator it=URLS.at(source_url)->begin(); it != URLS.at(source_url)->end(); it++, n++)
		{
			if(level >= MAX_CURL_DEPTH)
			{
				return 0;
			}
			else
			{
				mCurl(it->c_str(), level + 1);
			}
			previous = *it;
		}
	}	
	return 0;

}
int main (int argc, char* argv[])
{
	mCurl(argv[1], 0);	
}
