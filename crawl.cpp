/*
	Compile with: g++ crawl.cpp -o crawl -lcurl -std=c++11

*/
#include <iostream>
#include <stdio.h>
#include <cstdio>
#include <cstring>		// memset ()
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <curl/curl.h>		// http gathering

// link recognition (vs. file)
#include <sys/types.h>
#include <regex.h> 

#define REGEX_PATTERN "[a-zA-Z0-9./]*(.bmp|.gif|.jpg|.pdf|.png)"
//#define REGEX_PATTERN "/^[a-z](?:[-a-z0-9\+\.])*:(?:\/\/(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:])*@)?(?:\[(?:(?:(?:[0-9a-f]{1,4}:){6}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|::(?:[0-9a-f]{1,4}:){5}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:[0-9a-f]{1,4})?::(?:[0-9a-f]{1,4}:){4}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:[0-9a-f]{1,4}:[0-9a-f]{1,4})?::(?:[0-9a-f]{1,4}:){3}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:(?:[0-9a-f]{1,4}:){0,2}[0-9a-f]{1,4})?::(?:[0-9a-f]{1,4}:){2}(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:(?:[0-9a-f]{1,4}:){0,3}[0-9a-f]{1,4})?::[0-9a-f]{1,4}:(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:(?:[0-9a-f]{1,4}:){0,4}[0-9a-f]{1,4})?::(?:[0-9a-f]{1,4}:[0-9a-f]{1,4}|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3})|(?:(?:[0-9a-f]{1,4}:){0,5}[0-9a-f]{1,4})?::[0-9a-f]{1,4}|(?:(?:[0-9a-f]{1,4}:){0,6}[0-9a-f]{1,4})?::)|v[0-9a-f]+[-a-z0-9\._~!\$&'\(\)\*\+,;=:]+)\]|(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(?:\.(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3}|(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=@])*)(?::[0-9]*)?(?:\/(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))*)*|\/(?:(?:(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))+)(?:\/(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))*)*)?|(?:(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))+)(?:\/(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@]))*)*|(?!(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@])))(?:\?(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@])|[\x{E000}-\x{F8FF}\x{F0000}-\x{FFFFD}|\x{100000}-\x{10FFFD}\/\?])*)?(?:\#(?:(?:%[0-9a-f][0-9a-f]|[-a-z0-9\._~\x{A0}-\x{D7FF}\x{F900}-\x{FDCF}\x{FDF0}-\x{FFEF}\x{10000}-\x{1FFFD}\x{20000}-\x{2FFFD}\x{30000}-\x{3FFFD}\x{40000}-\x{4FFFD}\x{50000}-\x{5FFFD}\x{60000}-\x{6FFFD}\x{70000}-\x{7FFFD}\x{80000}-\x{8FFFD}\x{90000}-\x{9FFFD}\x{A0000}-\x{AFFFD}\x{B0000}-\x{BFFFD}\x{C0000}-\x{CFFFD}\x{D0000}-\x{DFFFD}\x{E1000}-\x{EFFFD}!\$&'\(\)\*\+,;=:@])|[\/\?])*)?$/i"  
#define DATA_DIR "data/"
#define INDENT_STEP 5
#define COLUMN_WIDTH 40
#define MAX_BUF_SIZE 4096
#define MAX_CURL_DEPTH 1

// unordered_map
// target url
//	unordered_set
//	<- origin url
//	<- origin url
// target url //	unordered_set
//	<- origin url
//	...
//std::unordered_multimap<std::string, std::string> URLS;
std::unordered_map<std::string, std::unordered_set<std::string>*> URLS;

struct UrlData
{
	std::string source;
	FILE * file;
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

std::string get_domain (std::string url)
{
	std::string dom = "";
	// Convert only the symbols, letters, and numbers
	// to strings. I had a hard time with an ASCII 13
	// (carriage regurn) being copied into the string.
	for (int i = 0; c_str[i] >= 33 && c_str[i] <= 126; i++)
	{
		if (c_str[i] == '\'' || c_str[i] == '/') break;
		dom += c_str[i];	
	} 
	return dom;

}
bool is_link (char * str, int level)
{
	int indent = level * INDENT_STEP;
	std::string url;
	regex_t regex;
	int reti;
	char buf [MAX_BUF_SIZE];

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
int get_urls (const char *input, struct UrlData *data)
{
	std::string url;
	const char * link = "http://";			// check for urls for all strings that 
							// begin with "http://"
	int cmp_index = 0;				// link[cmp_index] 
						
	char buf [MAX_BUF_SIZE];			// buf - for error messages
	int count = 0;					// count of urls found - get_urls returns this
	bool show_debug = false;
	
	for (int i = 0; input[i] != '\0'; i++)		// iterate through input
	{
		if (input[i] == link[cmp_index])	// when you find a match
		{
			if(cmp_index == 0 && show_debug) fprintf(stdout, "||%c", input[i]);
			cmp_index++;			// walk along match string
			if(link[cmp_index] == '\0')	// if you reach the end
			{				// of the match string
				i++;
				if(show_debug)fprintf(stdout, "<||%c", input[i]);

				// Copy the string AFTER the match into buf
				int b = 0;
				for (; input[i] != '\0' && input[i] != ' ' 
				    && input[i] != '"' && input[i] != '\n' 
				    && b < MAX_BUF_SIZE; b++, i++)
				{
					if(show_debug)fprintf(stdout, "%c", input[i]);
					buf[b] = input[i];
				}
				buf[b] = '\0';
				if(show_debug)fprintf(stdout, "||>");

				// then add it to the set of urls
				if (is_link(buf, data->level))
				{
					url = toString(buf);
					add_url (data->source, url);
				}
				std::memset(buf, 0, sizeof(buf));
				count++;
			}
		}
		else
		{
			if(show_debug)fprintf(stdout, "%c", input[i]);
			cmp_index = 0;
		}
	}	
	return count;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *data)
{
	int written = fwrite(ptr, size, nmemb, ((struct UrlData *)data)->file);
	get_urls((char *)ptr, (struct UrlData *)data);
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
	
	try
	{
		std::unordered_set<std::string> *target_urls = URLS.at(source_url);
	}
	catch (std::exception& e)
	{
		URLS.insert({{source_url, new std::unordered_set<std::string> ()}});
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
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

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
	std::string one = "abc";
	std::string two = "abc";
	mCurl(argv[1], 0);	
}
