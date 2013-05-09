#!/bin/sh
pushd output > /dev/null
echo "Start uploading ..."
#
#for alias "downloadtw" to work, add these to ~/.ssh/config
#=== ~/.ssh/config ===
#Host downloadtw
#    HostName <hostname>
#    Port <port>
#    User <username>
#
scp -r * downloadtw:.
echo "Upload completed ..."
echo "Uploaded files list:"
find -type f -exec openssl md5 '{}' \;
popd > /dev/null
echo -e "\033[41m                                                            \033[0m"
echo -e "\033[41m                                                            \033[0m"
echo -e "\033[41m                                                            \033[0m"
echo -e "\033[33;41m           Move files from home dir to /data/www/           \033[0m"
echo -e "\033[41m                                                            \033[0m"
echo -e "\033[41m                                                            \033[0m"
echo -e "\033[41m                                                            \033[0m"
