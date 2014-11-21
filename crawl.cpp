/*
	Compile with: g++ crawl.cpp -o crawl -lcurl -std=c++11

*/
#include <iostream>
#include <stdio.h>
#include <cstring>		// memset ()
#include <string>
#include <unordered_set>	// set of links 
#include <curl/curl.h>		// http gathering

// link recognition (vs. file)
#include <sys/types.h>
#include <regex.h>

#define MAX_BUF_SIZE 4096
#define MAX_CURL_DEPTH 2
int depth = 0;

std::unordered_set<std::string> urls;

bool is_link (char * str)
{
	regex_t regex;
	int reti;
	char buf [MAX_BUF_SIZE];

	reti = regcomp(&regex, "[a-zA-Z0-9./]*(.bmp|.gif|.jpg|.pdf|.png)", REG_EXTENDED);		// compile regex
	if (reti){fprintf(stderr, "Could not compile regex\n"); exit(1);}

	reti = regexec(&regex, str, 0, NULL, 0);
	reti = !reti;
	if(!reti)
	{
		fprintf(stdout, "[x] %s %d\n", str, reti);
		return true;
	}
	else if (reti == REG_NOMATCH)
	{
		fprintf(stdout, "[ ] %s %d\n", str, reti);
		return false;
	}
	else
	{
		regerror(reti, &regex, buf, sizeof(buf));
		fprintf(stderr, "Regex match failed: %s\n", buf);
		exit(1);
	}
}

void add_to_urls (char * str)
{
	std::string url = "";
	for (int i = 0; str[i] != '\0'; i++)
	{
		url += str[i];	
	} 
	if (url != "")
	{
		urls.insert(url);
	}	
}
int get_urls (char *input)
{
	const char * link = "http://";
	int cmp_index = 0;
	char buf [MAX_BUF_SIZE];
	int count = 0;
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
				if (is_link(buf)) add_to_urls(buf);
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

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int written = fwrite(ptr, size, nmemb, (FILE *)stream);
	get_urls((char *)ptr);
	return written;	
}

int mCurl (const char* url)
{
	CURL *curl_handle;
	CURLcode res;
	std::string headerFilename = std::to_string(depth) + "_" + std::string(url) + "_head.out";
	std::string bodyFilename = std::to_string(depth) + "_" + std::string(url) + "_body.out";
	FILE *headerFile;
	FILE *bodyFile;

	depth++;
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if(curl_handle)
	{

		/////////////////////////////////////////////
		//	SET UP CURL
		fprintf(stdout, "[%d] %s\n", depth, url);
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		//curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);	
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

		/////////////////////////////////////////////
		//	SET UP FILES
		headerFile = fopen(headerFilename.c_str(), "wb");
		if (headerFile == NULL)
		{
			curl_easy_cleanup(curl_handle);
			return -1;
		}
		bodyFile = fopen(bodyFilename.c_str(), "wb");
		if (bodyFile == NULL)
		{
			curl_easy_cleanup(curl_handle);
			return -1;
		}
		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, headerFile);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, bodyFile);


 		/////////////////////////////////////////////
		//	PERFORM THE CURL
		res = curl_easy_perform(curl_handle);


		
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		curl_easy_cleanup(curl_handle);
		
		for (std::unordered_set<std::string>::iterator it=urls.begin(); it != urls.end(); it++)
		{
			if(depth > MAX_CURL_DEPTH)
			{
				return 0;
			}
			else
			{
				fprintf(stdout, "trying url: %s\n", it->c_str());
				mCurl(it->c_str());
			}
		}
	}	
	return 0;

}
int main (int argc, char* argv[])
{
	mCurl(argv[1]);	
}
