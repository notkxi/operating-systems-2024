GROUP MEMBER #1:
   NAME: Kai Ibarrondo
   RUID: kai51
   RUID Number: 210004237

GROUP MEMBER #2:
   NAME: Davis Nguyen
   RUID: dhn28
   RUID Number: 210007132


PROJECT DESCRIPTION:
Multithreaded web crawler in C that includes the following functionalities:
• Multithreading: Uses multiple threads to fetch web pages concurrently.
• URL Queue: Implements a thread-safe queue to manage URLs that are pending to be fetched.
• HTML Parsing: Extracts links from the fetched web pages to find new URLs to crawl.
• Depth Control: Allows the crawler to limit the depth of the crawl to prevent infinite recursion.
• Synchronization: Implements synchronization mechanisms to manage access to shared resources among threads.
• Error Handling: Handles possible errors gracefully, including network errors, parsing errors, and dead links.
• Logging: Logs the crawler’s activity, including fetched URLs and encountered errors.


LIBRARIES USED:
• POSIX/Pthread
 - For working with multiple threads.

• libcurl
 - For fetching HTTP response.

• libxml2/libxml/HTMLparser
 - For parsing the URLs.

• glib
 - For creating hashmap to check for visisted URLs.


FEATURES DOCUMENTATION:
1.) Thread Management
 - For our multithreading approach, we used the C POSIX and pthread libraries to implement multiple worker 
   threads within a thread pool that fetch and process webpages in parallel.

2.) URL Queue
 - We implemented a thread-safe queue that stores URLs to be crawled.
 - Multiple threads are able to enqueue and dequeue the queue without data corruption.

3.) HTML Parsing
 - We used the libcurl library to fetch links. 
 - We implemented a ResponseData structure and write_callback function to retrieve responses when
   requesting for an HTTP in a link.
 - After a link is successfully fetched, we used the libxml2/libxml/HTMLparser libraries to parse
   the HTML content.
 - We implemented multiple functions that traverses a fetched link, parses the HTML content, and
   prints the extracted URLs from crawling the fetched link.

4.) Depth Control
 - We implemented depth control to limit how deep a crawler goes into a website.
 - The user is able to specify the maximum depth in the input.
 - The web crawler stops crawling once the depth is reached.

5.) Synchronization
 - We used mutexes as synchronization primitives to ensure that shared resources like the URL queue are
   accessed safely.

6.) Error Handling
 - We implemented error handling to manage network failures, invalid URLs, and other exceptions.
 - Error messages will print to the system for failures in various operations such as HTML parsing.

7.) Logging
 - We implemented logging of the progress of the web crawler, including which URLs have been visited and
   any errors encountered.
 - Other logging includes Thread IDs doing the work, which URLs are being processed, extracted links, 
   current depth levels, and when crawling has been completed for all threads.


CONTRIBUTIONS:
 • All group members worked together equally on all code.