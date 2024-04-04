# HaystackPPOS

Haystack was initially developed by Meta. It is an object storage system optimized for images, as Facebook serves over a million images per second at activity peaks. [Original paper here](https://research.facebook.com/publications/finding-a-needle-in-haystack-facebooks-photo-storage/). 

In a social medium as large as Facebook, each image is attached to metadata effectively identifying the image. This greatly simplifies image comparison operations, which in turn optimizes storage: by recognizing that two potentially distinct images sent by two different users on Facebook are actually the same, one can store the effectively unique image only once and query it whenever needed. At the scale of Facebook where the same image is often sent thousands if not millions of times, this approach drastically optimizes storage by avoiding unnecessary copies. 

Meta researchers had observed that their previous storage system incurred an excessive number of expensive disk operations looking for image metadata. Their solution was to reduce the size of metadata and store the metadata of all images in main memory, which yields cheaper metadata lookups. 

In this project, done within the context of a BSc second year course at EPFL teaching systems oriented programming in `C`, we had to implement - in groups of 2 - the core functionnalities of Haystack: the actual data structure to store metadata, as well as image registering, deletion, comparison, deduplication, and finally access to the image storage system via a web server.
