#ifndef _dO_Ob_ROBOTS_H_
#define _dO_Ob_ROBOTS_H_

namespace robots
{
	std::unordered_set<std::string> blacklist;
	std::unordered_set<std::string> visited;

	bool is_blacklisted (std::string url)
	{
		std::string adj_url = url;
		int http_pos = url.find("http://");
		if (http_pos == 0)
		{
			adj_url = url.substr(7, url.size() - 7);
		}
		else if ((http_pos = url.find("https://")) == 0)
		{
			adj_url = url.substr(8, url.size() - 8);
		}
		if (blacklist.count(adj_url) > 0)
		{
			std::cout << "[B] " << adj_url << std::endl;
			return true;
		}
		else return false;
	}

	std::string get_domain (std::string url)
	{
		std::string domain = url;
		int http_pos = url.find("http://");
		if (http_pos == 0)
		{
			domain = url.substr(7, url.size() - 7);
		}
		else if ((http_pos = url.find("https://")) == 0)
		{
			domain = url.substr(8, url.size() - 8);
		}
		int pos = domain.find_first_of("/");
		if (pos > 0)
		{
			domain = domain.substr(0, pos);
		}
		//std::cout << domain << std::endl;
		return domain;
	}

	static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *mem)
	{
		char * contents_ = (char *)contents;
	  size_t realsize = size * nmemb;
	  std::string *smem = (std::string *)mem;
	  *smem += std::string((char *)contents);
	  return realsize;
	}

	void print_chars (std::string str)
	{
		for (int i = 0; i < str.size(); i++)
		{
			std::cout << (int)str[i] << "|";
		}
		std::cout << std::endl;
	}

	void update_blacklist (std::string domain, std::string str)
	{
		std::string residual = str;
		std::string entry;
		int i = residual.find("Disallow: ");
		int i_end;
		while (i > -1)
		{
			//std::cout << "i = " << i << std::endl;
			i += 10;
			residual = residual.substr(i, residual.size() - 10);
			i_end = residual.find_first_of('\n');
			entry = residual.substr(0, i_end);		// defaults to remainder (e.g. no more newlines)
			//std::cout << "entry:" << entry << "(" << entry.size() << ") i_end:" << i_end << std::endl;
			std::cout << "[x] " << domain + entry << std::endl;
			blacklist.insert(domain + entry);

			if (residual[i_end] == 0 || i_end <= 0) break;
			//if (residual[i_end + 1] == 0) break;
			//std::cout << "i_end:" << i_end << " residual.size():" << residual.size() << " entry.size():" << entry.size() << std::endl;
			residual = residual.substr(i_end, residual.size() - entry.size());
			//std::cout << "residual: "; print_chars (residual); //<<residual << " (" << residual.size()<< ") " << std::endl;
			i = residual.find("Disallow: ");
			if (i == 0) break;
		}
	}



	std::string check (std::string url)
	{
		std::string* mem = new std::string ();

		std::string domain = get_domain(url);
		if(visited.count(domain) != 0) return "";
		visited.insert(domain);
		std::string robots_url = domain + "/robots.txt";

		CURL *curl_handle;
		CURLcode res;

		curl_global_init(CURL_GLOBAL_ALL);

		curl_handle = curl_easy_init();

		std::string heading = "[?] " + robots_url;
		std::cout << heading << std::endl;
		curl_easy_setopt(curl_handle, CURLOPT_URL, robots_url.c_str());
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)mem);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		/* get it! */
		res = curl_easy_perform(curl_handle);

		/* check for errors */
		if(res != CURLE_OK)
		{
			fprintf(stderr, "[!] no robots.txt %s\n",
			curl_easy_strerror(res));
			return "";
		}

		/* cleanup curl stuff */
		curl_easy_cleanup(curl_handle);

		update_blacklist(domain, *mem);
//		std::cout << *mem << std::endl;
//		for (int i = 0; (*mem)[i] != 0; i++)
//		{
//			std::cout << (*mem)[i] << "|";
//		}

		/* we're done with libcurl, so clean it up */
		curl_global_cleanup();
		return robots_url;
	}
}
#endif
