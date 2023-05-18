# SGX_Files

This is my repo for the Project GHOST work I've been doing, This inlcudes an SGX application that uses an enclave to send out web traffic to mimic actual user behaviour. It also includes some work I did visualizing the traffic that the app generated. Both are very simple versions and will be more robust in the future. 



To make the project, clone, ensure your machine has all intel SGX capabilities set up, then Make SGX=1, to run you need sudo privileges because you're opening a network device. run with: sudo ./app file1 ip1 file2 ip2 
