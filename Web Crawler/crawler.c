// Define the required feature test macro to enable certain POSIX functions.
#define _POSIX_C_SOURCE 200809L

// Include standard input/output functionality.
#include <stdio.h>
// Include standard library functionality, such as memory allocation and string manipulation.
#include <stdlib.h>
// Include the pthread library for multi-threading support.
#include <pthread.h>
// Include string manipulation functions.
#include <string.h>
// Include the boolean type definition.
#include <stdbool.h>
// Include system-specific functions and types.
#include <unistd.h>
// Include the libcurl library for performing HTTP requests.
#include <curl/curl.h>
// Include system-specific types.
#include <sys/types.h>
// Include GLib, a general-purpose utility library.
#include <glib.h>
// Include libxml2 for XML parsing functionality.
#include <libxml2/libxml/HTMLparser.h>

// Define the maximum number of threads.
#define MAX_THREADS 4
// Define the user agent string used in HTTP requests.
#define USER_AGENT "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)"

//Global hashmap to store urls that have been processed.
GHashTable *hashmap;
GMutex mutex;

//Structure to hold the HTTP response data.
struct ResponseData {
    char *data;
    size_t size;
};

// Define a structure for queue elements.
typedef struct URLQueueNode {
    char *url;
    char *base_url; // New field to store the base URL
    int depth;
    struct URLQueueNode *next;
} URLQueueNode;

// Define a structure for a thread-safe queue.
typedef struct {
    URLQueueNode *head, *tail;
    pthread_mutex_t lock;
} URLQueue;

// Thread pool structure
typedef struct {
    pthread_t threads[MAX_THREADS];  // Array to store thread IDs
    URLQueue *queue;                 // Pointer to the shared URL queue
    pthread_mutex_t lock;            // Mutex for thread synchronization
    pthread_cond_t task_available;   // Condition variable to signal availability of tasks
    int depth;                       // Depth limit for crawling
} ThreadPool;

void thread_pool_init(ThreadPool *pool, URLQueue *queue, int depth);
void thread_pool_submit(ThreadPool *pool);

void hashmap_init() {
    // Create a new hash table with string keys and NULL as the hash function and equality function
    hashmap = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
    // Initialize the mutex
    g_mutex_init(&mutex);
}

void hashmap_insert(const char* key) {
    g_mutex_lock(&mutex); // Acquire the mutex lock to ensure thread safety while modifying the hash map.
    // Insert the key into the hash table, using the key as the key and a NULL value.
    // g_strdup() is used to duplicate the key string because g_hash_table_insert() takes ownership of the key.
    g_hash_table_insert(hashmap, g_strdup(key), NULL);
    g_mutex_unlock(&mutex); // Release the mutex lock to allow other threads to access the hash map.
}

bool is_relative_url(const char *url) {
    // A relative URL does not start with "http://", "https://", "//", or "?"
    return !(strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0 ||
             strncmp(url, "//", 2) == 0 || url[0] == '?'|| url[0] == '/');
}

bool hashmap_contains(const char* key) {
    g_mutex_lock(&mutex);
    // Check if the key exists in the hash table
    bool found = g_hash_table_contains(hashmap, key);
    g_mutex_unlock(&mutex);
    return found;
}

void hashmap_cleanup() {
    // Free the memory used by the hash table
    g_hash_table_destroy(hashmap);
    // Destroy the mutex
    g_mutex_clear(&mutex);
}

// Initialize a URL queue.
void initQueue(URLQueue *queue) {
    queue->head = queue->tail = NULL;
    pthread_mutex_init(&queue->lock, NULL);
}

// Add a URL to the queue with its depth.
void enqueue(URLQueue *queue, const char *url, const char *base_url, int depth, ThreadPool *pool) {
    URLQueueNode *newNode = malloc(sizeof(URLQueueNode));
    newNode->url = strdup(url);
    newNode->base_url = strdup(base_url); // Store the base URL
    newNode->depth = depth;
    newNode->next = NULL;
    pthread_mutex_lock(&queue->lock);
    if (queue->tail) {
        queue->tail->next = newNode;
    } else {
        queue->head = newNode;
    }
    queue->tail = newNode;
    pthread_mutex_unlock(&queue->lock);

    // Signal that a task is available
    pthread_mutex_lock(&pool->lock);
    pthread_cond_signal(&pool->task_available);
    pthread_mutex_unlock(&pool->lock);
}

// Remove a URL from the queue.
URLQueueNode *dequeue(URLQueue *queue) {
    pthread_mutex_lock(&queue->lock);
    if (queue->head == NULL) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }
    URLQueueNode *temp = queue->head;
    queue->head = queue->head->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    pthread_mutex_unlock(&queue->lock);
    return temp;
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb; // Calculate the total size of the received data.

    // Cast the userp pointer to a struct ResponseData pointer to access the response data.
    struct ResponseData *response = (struct ResponseData *)userp;

    // Reallocate memory for the response data buffer to accommodate the newly received data.
    // The size is increased by the size of the received data plus one for the null terminator.
    response->data = realloc(response->data, response->size + realsize + 1);

    // Check if memory reallocation was successful.
    if (response->data == NULL) {
        fprintf(stderr, "Failed to allocate memory for response data\n");
        return 0; // Return 0 to indicate failure.
    }

    // Copy the received data into the response buffer starting from the current position.
    memcpy(&(response->data[response->size]), contents, realsize);

    // Update the size of the response data to reflect the addition of the newly received data.
    response->size += realsize;

    // Null-terminate the response string to ensure it is properly terminated.
    response->data[response->size] = '\0';

    return realsize; // Return the size of the received data.
}

char *extract_base_url(const char *url) {
    char *base_url = NULL;

    // Check if the URL starts with "http://" or "https://"
    const char *http_prefix = "http://";
    const char *https_prefix = "https://";

    const char *start_pos = strstr(url, http_prefix);
    if (!start_pos) {
        start_pos = strstr(url, https_prefix);
    }

    if (start_pos == url) { // Make sure "http://" or "https://" is at the beginning
        // Find the end position of the host part of the URL
        start_pos += strlen(http_prefix);
        const char *end_pos = strstr(start_pos, ".com");
        if (end_pos) {
            // Allocate memory for the base URL
            size_t base_length = end_pos - url + strlen(".com"); // Adjust to include ".com"
            base_url = (char *)malloc((base_length + 1) * sizeof(char));
            if (base_url) {
                // Copy the host part of the URL to the base URL
                strncpy(base_url, url, base_length);
                base_url[base_length] = '\0'; // Null-terminate the string
            }
        } else {
            // If no ".com" component, base URL is the same as the input URL
            base_url = strdup(url);
        }
    }

    return base_url;
}
void process_href(URLQueue *queue, xmlChar *href, const char *base_url, int depth, ThreadPool *pool) {
    if (href != NULL) {
        // Check if the href is a relative URL
        if (is_relative_url((char *)href)) {
            // Concatenate the base URL with the extracted href.
            char *full_url = malloc(strlen(base_url) + xmlStrlen(href) + 2);
            if (full_url != NULL) {
                strcpy(full_url, base_url);
                strcat(full_url, "/");
                strcat(full_url, (char *)href);
                if (!hashmap_contains(full_url)) {
                    hashmap_insert(full_url);
                    enqueue(queue, full_url, base_url, depth + 1, pool);; // Decrease depth and pass thread ID
                    printf("Extracted relative href: %s (Thread ID: %lu) (Depth: %d)\n", full_url, pthread_self(), depth);
                    thread_pool_submit(pool); // Submit work to the thread pool
                } else {
                    printf("This link has already been crawled!: %s\n", full_url);
                    free(full_url);
                }
            } else {
                fprintf(stderr, "Failed to allocate memory for full URL\n");
            }
        } else if (((char *)href)[0] == '/' && ((char *)href)[1] != '/') {
            // Concatenate the base URL with the extracted href.
            char *full_url = malloc(strlen(base_url) + xmlStrlen(href) + 1);
            if (full_url != NULL) {
                strcpy(full_url, base_url);
                strcat(full_url, (char *)href);
                if (!hashmap_contains(full_url)) {
                    hashmap_insert(full_url);
                    enqueue(queue, full_url, base_url, depth + 1, pool);// Decrease depth and pass thread ID
                    printf("Extracted relative href: %s (Thread ID: %lu) (Depth: %d)\n", full_url, pthread_self(), depth);
                    thread_pool_submit(pool); // Submit work to the thread pool
                } else {
                    printf("This link has already been crawled!: %s\n", full_url);
                    free(full_url);
                }
            } else {
                fprintf(stderr, "Failed to allocate memory for full URL\n");
            }
        } else if (((char *)href)[0] == '/' && ((char *)href)[1] == '/') {
            // Append 'https:' to the href and enqueue the URL
            char *full_url = malloc(strlen(base_url) + xmlStrlen(href) + 1);
            if (full_url != NULL) {
                strcpy(full_url, "https:");
                strcat(full_url, (char *)href);
                if (!hashmap_contains(full_url)) {
                    hashmap_insert(full_url);
                    enqueue(queue, full_url, base_url, depth + 1, pool); // Decrease depth and pass thread ID
                    printf("Extracted relative href: %s (Thread ID: %lu) (Depth: %d)\n", full_url, pthread_self(), depth);
                    thread_pool_submit(pool); // Submit work to the thread pool
                } else {
                    printf("This link has already been crawled!: %s\n", full_url);
                    free(full_url);
                }
            } else {
                fprintf(stderr, "Failed to allocate memory for full URL\n");
            }
        } else if (((char *)href)[0] == '?') {
            // Append the base URL to the href and enqueue the URL
            char *full_url = malloc(strlen(base_url) + xmlStrlen(href) + 1);
            if (full_url != NULL) {
                strcpy(full_url, base_url);
                strcat(full_url, "/");
                strcat(full_url, (char *)href);
                if (!hashmap_contains(full_url)) {
                    hashmap_insert(full_url);
                    enqueue(queue, full_url, base_url, depth + 1, pool); // Decrease depth and pass thread ID
                    printf("Extracted relative href: %s (Thread ID: %lu) (Depth: %d)\n", full_url, pthread_self(), depth);
                    thread_pool_submit(pool); // Submit work to the thread pool
                } else {
                    printf("This link has already been crawled!: %s\n", full_url);
                    free(full_url);
                }
            } else {
                fprintf(stderr, "Failed to allocate memory for full URL\n");
            }
        } else {
            // Otherwise, enqueue the href as it is.
            char *href_str = (char *)href;
            // Check if the href is not NULL before enqueuing
            if (href_str != NULL && !hashmap_contains(href_str)) {
                hashmap_insert(href_str);
                enqueue(queue, href_str, base_url, depth + 1, pool); // Decrease depth and pass thread ID
                printf("Extracted href: %s (Thread ID: %lu) (Depth: %d) \n", href_str, pthread_self(), depth);
                thread_pool_submit(pool); // Submit work to the thread pool
            } else if (href_str != NULL) {
                printf("This link has already been crawled!: %s\n", href_str);
                free(href);
            }
        }
    }
}

void recursive_parse_html(htmlNodePtr node, URLQueue *queue, const char *base_url, int depth, ThreadPool *pool) {
    // Iterate through each HTML node
    for (htmlNodePtr cur_node = node; cur_node; cur_node = cur_node->next) {
        // Check if the node is an XML element
        if (cur_node->type == XML_ELEMENT_NODE) {
            // Check if the node is an 'a' tag (hyperlink)
            if (!xmlStrcmp(cur_node->name, (const xmlChar *)"a")) {
                // Get the 'href' attribute of the 'a' tag
                xmlChar *href = xmlGetProp(cur_node, (const xmlChar *)"href");
                if (href != NULL) {
                    // Process the extracted hyperlink and enqueue it
                    process_href(queue, href, base_url, depth, pool);
                }
            }
        }
        // Recursively process child nodes
        recursive_parse_html(cur_node->children, queue, base_url, depth, pool);
    }
}

void parse_html(URLQueue *queue, const char *html_content, const char *base_url, int depth, ThreadPool *pool) {
    // Parse the HTML content into a DOM tree using libxml2
    htmlDocPtr document = htmlReadMemory(html_content, strlen(html_content), NULL, NULL, HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
    if (document == NULL) {
        // Print error message if parsing fails
        fprintf(stderr, "Failed to parse HTML content\n");
        return;
    }

    // The parsing is performed in a depth-first manner, starting from the root node of the HTML document.
    // The root node is obtained using xmlDocGetRootElement().

    // Traverse the DOM tree using depth-first search (DFS), processing each node recursively.
    // The recursive_parse_html() function is called to extract hyperlinks and enqueue them for further processing.
    recursive_parse_html(xmlDocGetRootElement(document), queue, base_url, depth, pool);

    // Free the memory allocated for the DOM tree
    xmlFreeDoc(document);
}

// Function to fetch and process a URL
/**
 * @brief Function executed by worker threads to fetch and process URLs.
 *
 * This function is executed by worker threads in the thread pool to fetch and process URLs.
 * It continuously dequeues URLs from the URL queue, fetches their content using libcurl, and
 * processes the HTML content to extract hyperlinks. The extracted hyperlinks are then enqueued
 * for further processing.
 *
 * @param arg A pointer to the ThreadPool structure containing thread pool information.
 * @return NULL upon completion of the task.
 */
void *fetch_url(void *arg) {
    ThreadPool *pool = (ThreadPool*)arg; // Cast the argument to ThreadPool pointer
    URLQueue *queue = pool->queue; // Retrieve URL queue from ThreadPool
    int depth = pool->depth; // Retrieve the maximum depth from ThreadPool

    // Main loop to continuously fetch and process URLs until depth is to url depth
    while (true) {
        pthread_mutex_lock(&pool->lock); // Acquire the thread pool lock

        // Wait while the URL queue is empty
        while (queue->head == NULL) {
            pthread_cond_wait(&pool->task_available, &pool->lock); // Wait for task availability
        }


        pthread_mutex_unlock(&pool->lock); // Unlock the mutex

        // Dequeue the next URL to process
        URLQueueNode *node = dequeue(queue);

        // Check if the dequeued node is NULL (indicating an empty queue)
        if (!node) {
            continue; // Continue to the next iteration of the loop
        }

        int curr_depth = node->depth; // Retrieve the current depth of the URL
        if (curr_depth == depth) {
            // If the current depth reaches the maximum depth, free resources and exit thread
            free(node->url);
            free(node);
            pthread_exit(NULL);
        }

        // Fetch URL content using libcurl
        struct ResponseData response;
        response.data = NULL;
        response.size = 0;

        char *url = strdup(node->url); // Duplicate the URL string
        char *base_url = node->base_url; // Retrieve the base 0*URL from the URLNode

        // Extract base URL from the URL if not provided
        if (base_url == NULL && (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0)) {
            base_url = extract_base_url(url);
        }

        // Initialize libcurl handle
        CURL *curl = curl_easy_init();
        if (!curl) {
            // Print error message if libcurl initialization fails
            fprintf(stderr, "Failed to initialize cURL\n");
            free(node->url);
            free(node);
            continue;
        }

        // Set libcurl options for HTTP request
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT); // Set user-agent header
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // Set timeout for request
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback); // Set write callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response); // Set write data
        curl_easy_setopt(curl, CURLOPT_URL, url); // Set URL for request

        // Perform HTTP request
        CURLcode request_result = curl_easy_perform(curl);
        if (request_result != CURLE_OK) {
            // Print error message if HTTP request fails
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(request_result));
        } else {
            // Check if HTML content is received
            if (response.data == NULL || response.size == 0) {
                // Print error message if no HTML content received
                fprintf(stderr, "Error: No HTML content received for URL: %s\n", url);
            } else {
                // Print status and process received HTML content
                printf("\nThread ID: %lu is processing URL: %s\n", pthread_self(), url);
                printf("Parsing HTML content...\n");
                parse_html(queue, response.data, base_url, curr_depth, pool); // Parse HTML content
            }
        }

        // Cleanup: free resources and memory
        printf("Thread %lu: Finished processing URL: %s\n", pthread_self(), node->url);
        free(response.data); // Free response buffer
        free(url); // Free duplicated URL string
        free(node->url); // Free URL stored in URLNode
        free(node); // Free URLNode
    }

    return NULL;
}


void thread_pool_init(ThreadPool *pool, URLQueue *queue, int depth) {
    // Initialize the mutex lock for thread synchronization
    pthread_mutex_init(&pool->lock, NULL);

    // Initialize the condition variable for signaling task availability
    pthread_cond_init(&pool->task_available, NULL);

    // Assign the task queue, maximum depth, and
    pool->queue = queue;
    pool->depth = depth;


    // Create worker threads to populate the thread pool
    for (int i = 0; i < MAX_THREADS; i++) {
        // Create a new thread and assign the fetch_url function as its entry point
        // Pass the ThreadPool pointer (pool) as the argument to the fetch_url function
        pthread_create(&pool->threads[i], NULL, fetch_url, (void*) pool);
    }
}

// Submit a task to the thread pool
void thread_pool_submit(ThreadPool *pool) {
    pthread_mutex_lock(&pool->lock); // Acquire the thread pool mutex lock

    // Broadcast the task_available signal to wake up all waiting threads
    pthread_cond_broadcast(&pool->task_available);

    pthread_mutex_unlock(&pool->lock); // Release the thread pool mutex lock
}

/**
 * @brief The main function responsible for initiating the web crawler.
 *
 * This function serves as the entry point for the web crawler program. It parses command-line arguments,
 * initializes necessary data structures, creates a thread pool, enqueues starting URLs, and waits for
 * worker threads to complete their tasks before exiting.
 *
 * @param argc An integer representing the number of command-line arguments.
 * @param argv An array of strings containing the command-line arguments.
 * @return An integer indicating the exit status of the program.
 */
int main(int argc, char *argv[]) {
    // Check if the correct number of command-line arguments is provided
    if (argc < 3) {
        // If insufficient arguments, display usage information and exit with status 1
        printf("Usage: %s <starting-url> <depth>\n", argv[0]);
        return 1;
    }

    // Extract the depth from the command-line arguments
    int depth = atoi(argv[argc - 1]);

    if (depth < 0) {
        printf("Invalid depth: Depth cannot be negative.\n");
        return 1;
    }

    // Extract the base URL from the first command-line argument
    char *base_url = extract_base_url(argv[1]);

    // Initialize the URL queue and hash map for tracking visited URLs
    URLQueue queue;
    initQueue(&queue);
    hashmap_init();

    // Print status message indicating the creation of the thread pool
    printf("Creating thread pool...\n");

    // Initialize the thread pool with the specified depth and associated URL queue
    ThreadPool pool;
    thread_pool_init(&pool, &queue, depth);

    // Add tasks to the thread pool for processing
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_pool_submit(&pool);
    }

    // Enqueue the provided starting URL with depth 0
    enqueue(&queue, argv[1], base_url, 0, &pool);

    // Print status message indicating the creation of the thread pool
    printf("Thread pool created with %d threads.\n\n", MAX_THREADS);

    // Wait for all worker threads to complete their tasks before exiting
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(pool.threads[i], NULL);
    }

    // Print status message indicating the completion of all threads
    printf("All threads have completed.\n");

    // Cleanup and program termination.

    return 0;
}
