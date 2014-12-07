#!/bin/bash

URL=$1
LIMIT=$2
DESTINATION=$3

echo "URL:$URL LIMIT:$LIMIT"

./crawl $URL $LIMIT

HDFS_STMT="$HADOOP_HOME/bin/hdfs -put data/* $DESTINATION"
