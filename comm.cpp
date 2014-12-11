#include <cstring>
#include <iostream>	
#include <curl/curl.h>	
#include <stdlib.h>	// realloc, malloc

struct MemoryStruct {
  char *memory;
  size_t size;
};

const char data[]="{\"statements\" : [{\"statement\" : \"CREATE (a) RETURN id(a)\"} ]}";

struct WriteThis {
  const char *readptr;
  int sizeleft;
};

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *pooh = (struct WriteThis *)userp;

  if(size*nmemb < pooh->sizeleft) {
    *(char *)ptr = pooh->readptr[0]; /* copy one single byte */ 
    pooh->readptr++;                 /* advance pointer */ 
    pooh->sizeleft--;                /* less data left */ 
    return 1;                        /* we return 1 byte at a time! */ 
  }

  return 0;                          /* no more data left to deliver */ 
}


static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

std::string POST (std::string post_msg, std::string url)
{
	CURL *curl_handle;
	CURLcode res;
	struct curl_slist *headers = NULL;
 
	struct MemoryStruct chunk;
 struct WriteThis pooh;

  pooh.readptr = data;
  pooh.sizeleft = strlen(data);

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	//headers = curl_slist_append(headers, "POST http://localhost:7474/db/data/transaction/commit");
	headers = curl_slist_append(headers, "Accept: application/json; charset=UTF-8");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	//headers = curl_slist_append(headers, data);

	curl_handle = curl_easy_init();
	if(curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());			// url set
		
		curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);		// send POST message
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
		//curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);	// write response to memory
		//curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);		// write response memory is &chunk
		
    /* we want to use our own read function */ 
    //curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_callback);

    /* pointer to pass to our read function */ 
    //curl_easy_setopt(curl_handle, CURLOPT_READDATA, &pooh);

    /* get verbose debug output please */ 
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

		res = curl_easy_perform(curl_handle);
		if(res != CURLE_OK)
		{
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		}
		else
		{
			//std::cout << "Received " << chunk.size << " bytes.\n";
		}
		curl_easy_cleanup(curl_handle);	
	}
	if(chunk.memory) free(chunk.memory);
	curl_global_cleanup();
	return "";
}

int main(void)
{
	POST ("POST http://localhost:7474/db/data/transaction/commit", "http://localhost:7474/db/data/transaction/commit");
}
