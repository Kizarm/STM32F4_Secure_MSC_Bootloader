all: disk.img
./mnt:
	mkdir ./mnt
# Obraz disku si pripravime standardni unixovou metodou. Minimalni velikost je 66x512 byte
disk.img: ./mnt
	dd if=/dev/zero of=disk.img bs=512 count=65536
	sudo losetup /dev/loop0 ./disk.img
	sudo mkfs.vfat -v -F 32 -f 2 -S 512 -s 8 -n MUSIC /dev/loop0
	sudo mount /dev/loop0 ./mnt
	sudo cp -r ~/Hudba/BMOLL/*.mp3 ./mnt
	sudo sync
	sudo sleep 1
	sudo umount ./mnt
	sudo losetup -d /dev/loop0
