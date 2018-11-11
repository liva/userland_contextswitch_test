.PHONY: docker

run: worker dispatcher
	docker run --rm -v $(CURDIR):/workdir -w /workdir -it --cap-add=SYS_PTRACE --security-opt="seccomp=unconfined" ulcs sh -c "./dispatcher"

dispatcher: dispatcher.cc
	docker run --rm -v $(CURDIR):/workdir -w /workdir -it ulcs g++ -O0 -Wall --std=c++14 -o dispatcher dispatcher.cc

worker: worker.cc int.S
	docker run --rm -v $(CURDIR):/workdir -w /workdir -it ulcs g++ -O0 -Wall --std=c++14 -o worker worker.cc int.S

docker:
	docker build --no-cache -t ulcs docker