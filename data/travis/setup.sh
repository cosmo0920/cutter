#!/bin/sh

set -e
set -x

distribution=$(lsb_release --short --id | tr 'A-Z' 'a-z')
code_name=$(lsb_release --short --codename)
component=main
apt_url_base=http://downloads.sourceforge.net/project/cutter
cat <<EOF | sudo tee /etc/apt/sources.list.d/cutter.list
deb ${apt_url_base}/${distribution}/ ${code_name} ${component}
deb-src ${apt_url_base}/${distribution}/ ${code_name} ${component}
EOF

sudo apt-get update
sudo apt-get -y --allow-unauthenticated install cutter-keyring
sudo apt-get update
sudo apt-get -y -V install cutter-testing-framework
