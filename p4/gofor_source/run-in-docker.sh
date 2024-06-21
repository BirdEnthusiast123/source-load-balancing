#get the current directory
DIR="$( cd "$( dirname "$0" )" && pwd )"

#build 
docker build -t gofor-experiments "$DIR"

#run
docker run -v "$DIR/":/home/user -w /home/user/evaluation -p 8889:8888 gofor-experiments


#On windows
#uncomment the line in the Dockerfile
#then build with 
#docker.exe build -t gofor-experiments "$DIR"

#then run (it is readonly, so modifying the notebook has no effect on the notebook on the host)
#docker.exe run -w /home/user/evaluation -p 8888:8888 gofor-experiments


# docker run -it -v "$PWD/":/home/user -w /home/user/evaluation gofor-experiments