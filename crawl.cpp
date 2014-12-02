/*
	Compile with: g++ crawl.cpp -o crawl -lcurl -std=c++11

*/
#include <cstring>			// memcpy () - aggregating data received by CURLOPT_WRITEFUNCTION (curl.h)
#include <unordered_map>	// for URLS - data structure to track all urls, their source and destinations
#include <unordered_set>	// for URLS
#include <curl/curl.h>		// http gathering
#include <fstream>			// write URL_directory, write_file ()
#include <queue>
#include <signal.h>			// block sigterm
#include <regex.h> 			// link recognition (vs. file)
#include <utility>
#include "Parser.h"
#include "robots.h"

#define REGEX_PATTERN "[a-zA-Z0-9./]*(.bmp|.gif|.jpg|.pdf|.png)"	// link recognition - regex.h
#define DATA_DIR "data/"		// fwrite () into this directory
#define INDENT_STEP 5			// print_column () - indent columns
#define COLUMN_WIDTH 80			// print_column () - width of block to print
#define HTML_BUF_SIZE 2000000	// for UrlData buffer (stores html data)
#define MSG_BUF_SIZE 4096		// link recognition (regex.h) error
int TIME_LIMIT = 60;

std::unordered_map<int, std::unordered_set<int>*> URLS;
std::queue<std::string> URL_queue;
int MAX_USED = 0;

class Directory
{
private:
	std::unordered_map<int, std::string> standard;
	std::unordered_map<std::string, int> reverse;
	std::ofstream ofile;
public:
	bool insert (int key, std::string value)
	{
		std::pair<std::string, int> rev_entry (value, key);
		if(reverse.insert(rev_entry).second)
		{
			//std::pair<int, std::string> entry (key, value);
			//standard.insert(entry);
			standard[key] = value;
			return true;
		}
		else return false;
	}

	std::string get_value (int key)
	{
		return standard.at(key);
	}

	int get_key (std::string value)
	{
		return reverse.at(value);
	}

	int size ()
	{
		return standard.size();
	}

	void write_file (std::string ofile_name)
	{
		ofile.open(ofile_name);
		if(!ofile.is_open())
		{
			std::cerr << "[!] " << ofile_name << " did not open.\n";
			return;
		}
		else
		{
			for (std::unordered_map<int, std::string>::iterator it = standard.begin(); it != standard.end(); it++)
			{
				ofile << it->first << " " << it->second << std::endl;
			}
		}
		ofile.close();
		std::cout << "[*] " << ofile_name << " written.\n";
	}
} URL_directory;

struct UrlData
{
	std::string source;
	FILE * file;
	char buffer [HTML_BUF_SIZE];
	int buf_size = 0;
	//int level;
};
//void print_indent (int indent)
//{
//	for (int i = 0; i < indent; i++)
//	{
//		fprintf(stdout, " ");
//	}
//}
void print_column (std::string str, int width)
{
	for (int i = 0; i < width && str[i] != '\0'; i++)
	{
		std::cout << str[i];
	}
	std::cout << std::endl;
}

std::string fix_rel_url (std::string source_url, std::string new_url)
{
	//std::cout << "source: " << source_url << " new: " << new_url << std::endl;
	if (new_url.front() == '/')
	{
		if (source_url.back() == '/')
		{
			new_url = source_url.substr(0,source_url.size() - 1) + new_url;
		}
		else
		{
			new_url = source_url + new_url;
		}
	}
	else if (new_url.substr(0,3) == "../")
	{
		if (source_url.back() == '/') source_url = source_url.substr(0, source_url.size() - 1);
		while (new_url.substr(0,3) == "../")
		{
			int pos = source_url.find_last_of("/");
			source_url = source_url.substr(0, pos);
			new_url = new_url.substr(3, new_url.size() - 3);
		}
		new_url = source_url + "/" + new_url;
	}
	else if (new_url.front() == '#')
	{
		if (source_url.back() == '/') new_url = source_url + new_url;
		else new_url = source_url + "/" + new_url;
	}
	//std::cout << "end source: " << source_url << " new: " << new_url << std::endl;
	return new_url;
}



bool is_link (char * str, int level)
{
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
		//print_indent(level * INDENT_STEP);print_column(url.c_str(), COLUMN_WIDTH);
		return true;
	}
	else if (reti == REG_NOMATCH)
	{	
		url = "[ ] "; url += str;
		//print_indent(level * INDENT_STEP);print_column(url.c_str(), COLUMN_WIDTH);
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

int mCurl (std::string source_url, int nth_curl)
{
	int source_url_id = URL_directory.get_key(source_url);

	UrlData header_data;
	UrlData body_data;

	CURL *curl_handle;
	CURLcode res = CURLE_OK;

	std::string headerFilename = DATA_DIR + std::to_string(source_url_id) + "_head.out";
	std::string bodyFilename = DATA_DIR + std::to_string(source_url_id) + "_body.out";


	std::unordered_set<int> *target_URLS;

	if (URLS.count(source_url_id) == 0)
	{
		URLS[source_url_id] = new std::unordered_set<int> ();
	}
	target_URLS = URLS[source_url_id];

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();

	if(curl_handle)
	{

		/////////////////////////////////////////////
		//	SET UP CURL
		std::string heading = std::string("[" +  std::to_string(nth_curl) + "]-> " + source_url + "(" + std::to_string(source_url_id) + ")");
		print_column (heading, COLUMN_WIDTH);
		curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(curl_handle, CURLOPT_URL, source_url.c_str());
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_body);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, TIME_LIMIT);

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

		header_data.source = source_url;
		body_data.source = source_url;

		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &header_data);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body_data);

 		/////////////////////////////////////////////
		//	PERFORM THE CURL
		res = curl_easy_perform(curl_handle);
		
		// 1. Block SIGINT
		sigset_t old_mask, to_block;
		sigemptyset(&to_block);
		sigaddset(&to_block, SIGINT);
		sigprocmask(SIG_BLOCK, &to_block, &old_mask);

		fclose(header_data.file);
		fclose(body_data.file);

		// 3. Restore signal handling
		sigprocmask(SIG_SETMASK, &old_mask, NULL);

		if (res != CURLE_OK)
		{
			fprintf(stdout, "[!]-> %s\n", curl_easy_strerror(res));

			remove(headerFilename.c_str());
			remove(bodyFilename.c_str());

			std::cerr << "[!] files removed." << std::endl;
		}
		curl_easy_cleanup(curl_handle);

		std::list<std::string> *new_URLS = new std::list<std::string>();
		Parser Parser_((char *)body_data.buffer);
		Parser_.set_debug(false);
		Parser_.process();
		Parser_.get_attribute_values("href", new_URLS);

		std::string new_url;
		std::string robots_url;
		for (std::list<std::string>::iterator it=new_URLS->begin(); it != new_URLS->end(); it++)
		{
			int new_id = URL_directory.size();
			new_url = fix_rel_url(source_url, *it);
			std::cout << new_url << std::endl;
			robots_url = robots::check(new_url);			// get the proposed robots url: domain/robots.txt
			if(robots_url != "")							// and perform curl on it (check())
			{												// it will return "" if no robots url
				URL_directory.insert(new_id, robots_url);	// if there is a robots url, insert the domain/robots.txt into the
				new_id++;									// directory so we dont check() it again.
			}
			if (!robots::is_blacklisted(new_url))			// if the new_url wasn't blacklisted in a robots.txt
			{
				if (URL_directory.insert(new_id, new_url))	// and if it is a new url (insert will return true)
				{
					target_URLS->insert(new_id);			// then add the new_id to the list of target_URLS
				}
			}
		}
		delete new_URLS;
		target_URLS = NULL;

		std::string footer = "[+] " + std::to_string(source_url_id) + " urls found.";
		print_column (footer.c_str(), COLUMN_WIDTH);
		for (std::unordered_set<int>::iterator it=URLS.at(source_url_id)->begin(); it != URLS.at(source_url_id)->end(); it++)
		{
			std::string url = URL_directory.get_value(*it);
			URL_queue.push(url);
		}
	}	
	else
	{
		fprintf(stderr, "[!] could not initialize curl.\n");
	}
	return res;
}

void clean_up ()
{
	//std::unordered_map<int, std::unordered_set<int>*> URLS;
	for (std::unordered_map<int, std::unordered_set<int>*>::iterator it = URLS.begin(); it != URLS.end(); it++)
	{
		delete it->second;
	}
}
int main (int argc, char* argv[])
{
	int n = 0;
	int max_curls;
	std::stringstream ss; ss << argv[2];
	ss >> max_curls;
	std::string url = std::string(argv[1]);
	robots::check(url);
	if (robots::is_blacklisted(url)) return 0;
	URL_directory.insert(0, argv[1]);
	URL_queue.push(argv[1]);
	while (URL_queue.size() > 0 && n <= max_curls)
	{
		if(mCurl(URL_queue.front(), n) == CURLE_OK) n++;
		URL_queue.pop();
	}
	std::string ofile_name = DATA_DIR;
	ofile_name += "directory.txt";
	URL_directory.write_file(ofile_name);
	clean_up();
}
