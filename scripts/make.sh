cd ..
export ROOT_DIR=`pwd`

cd /tmp/
git clone https://github.com/sisoputnfrba/so-commons-library.git
cd so-commons-library
sudo make install

sudo apt-get install libreadline6 libreadline6-dev

cd $ROOT_DIR
rm -rf `find . -name build -type d`

cd $ROOT_DIR/ipc
sudo make all
sudo make install

cd $ROOT_DIR/yama
mkdir build
gcc src/*.c -o build/yama -lipc -lcommons -lpthread

cd $ROOT_DIR/filesystem
mkdir build
gcc src/*.c -o build/filesystem -lipc -lcommons -lpthread -lm -lreadline

cd $ROOT_DIR/master
mkdir build
gcc src/*.c -o build/master -lcommons -lpthread -lm -lipc

cd $ROOT_DIR/node/datanode
mkdir build
gcc src/*.c -o build/datanode -lipc -lcommons

cd $ROOT_DIR/node/worker
mkdir build
gcc src/*.c -o build/worker -lipc -lcommons
