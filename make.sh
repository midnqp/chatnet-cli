#!/bin/bash
 gcc -Wall -fsanitize=address -g ./client.c -lleveldb -o dist/chatnet-client
