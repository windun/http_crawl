/*
	Compile with: g++ crawl.cpp -o crawl -lcurl -std=c++11

*/
#include <iostream>
#include <stdio.h>
#include <cstdio>
#include <cstring>		// memset ()
#include <string>
#include <unordered_set>	// set of links 
#include <curl/curl.h>		// http gathering

// link recognition (vs. file)
#include <sys/types.h>
#include <regex.h>

#define MAX_BUF_SIZE 4096
#define MAX_CURL_DEPTH 2

struct UrlData
{
	FILE * file;
	std::unordered_set<std::string> urls;
	int level;
};
void print_column (int indent, char * str, int width)
{
	for (int i = 0; i < width && str[i] != '\0'; i++)
	{
		fprintf(stdout, "%c", str[i]);
	}
	fprintf(stdout, "\n");
}
bool is_link (char * str, int level)
{
	int indent = level * 5;
	regex_t regex;
	int reti;
	char buf [MAX_BUF_SIZE];

	// match image files, is_link() should return false for
	// thee matches, so we will negate the result
	reti = regcomp(&regex, "[a-zA-Z0-9./]*(.bmp|.gif|.jpg|.pdf|.png)", REG_EXTENDED);		// compile regex
	if (reti){fprintf(stderr, "Could not compile regex\n"); exit(1);}

	reti = regexec(&regex, str, 0, NULL, 0);
	//reti == 0 if there was a match, 1 if no match
	reti = !reti;	// negate match result
	if(!reti)
	{
		print_column(indent, str,40);
		return true;
	}
	else if (reti == REG_NOMATCH)
	{
		//fprintf(stdout, "[ ] %s %d\n", str, reti);
		return false;
	}
	else
	{
		regerror(reti, &regex, buf, sizeof(buf));
		fprintf(stderr, "Regex match failed: %s\n", buf);
		exit(1);
	}
}

std::string toString (char * c_str)
{
	std::string str = "";
	for (int i = 0; c_str[i] != '\0'; i++)
	{
		str += c_str[i];	
	} 
	return str;
}

int get_urls (char *input, struct UrlData *data)
{
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
					data->urls.insert(toString(buf));
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

int mCurl (const char* url, int level)
{
	struct UrlData header_data;
	struct UrlData body_data;

	CURL *curl_handle;
	CURLcode res;
	std::string headerFilename = std::to_string(level) + "_" + std::string(url) + "_head.out";
	std::string bodyFilename = std::to_string(level) + "_" + std::string(url) + "_body.out";
	
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if(curl_handle)
	{

		/////////////////////////////////////////////
		//	SET UP CURL
		fprintf(stdout, "[%d] %s\n", level, url);
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
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
		header_data.level = level;
		body_data.level = level;
		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &header_data);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body_data);


 		/////////////////////////////////////////////
		//	PERFORM THE CURL
		res = curl_easy_perform(curl_handle);


		
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			remove(headerFilename.c_str());
			remove(bodyFilename.c_str());
		}
		curl_easy_cleanup(curl_handle);
		
		for (std::unordered_set<std::string>::iterator it=body_data.urls.begin(); it != body_data.urls.end(); it++)
		{
			if(level > MAX_CURL_DEPTH)
			{
				return 0;
			}
			else
			{
				fprintf(stdout, "trying url: %s\n", it->c_str());
				mCurl(it->c_str(), level + 1);
			}
		}
	}	
	return 0;

}
int main (int argc, char* argv[])
{
	mCurl(argv[1], 0);	
}
