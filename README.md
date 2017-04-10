# EasyDeployer
Simple tool for software distribution inside entreprises* in domain environment only.

It operate in the same way as PsExec(sysinternal tools) , it pushes a service to the remote computer then it will be started in system context, we communicate with it service via pipename (sending msi, exe commands ... ect).

PsExec delete the service after the command execution , when easydeployer don't (it will be conserved for future use ), unless you want too (for that you can use option 6 and delete the service).

How to use 

1-Create a File with all the computers name or ip adresses (you must be the domain administrator for those computers ).
2-Create a read only shared folder that can be accessed by all authenticated users.
3-Open the easydeployer app you will be see the following options 
 What do you want to do

1. INSTALL EASYDEPLOYER SERVICE AND START IT
2. ENNUMERATE SERVICES
3. DELETE A SERVICE
4. DEPLOY A SOFTWARE
5. START A SERVICE
6. STOP A SERVICE

4-For the first time run , you have to install the easydeployer service (that you have compiled from the folder https://github.com/BENSEBTI/EasyDeployer/tree/master/EasyDeployer  ).  (for this step you must install the service one by one , we are automating that, drop me a mail if you want the automatic one )
*the easydeployer service must be in a shared accessible folder 

5- After making sure that the easydeployer service is installed on the computers (you can verify that by executing ENNUMERATE SERVICES on those computers to make sure ).
 - Choose 4. DEPLOY A SOFTWARE and provide the computers txt file , and the application path , choose between exe and msi
 execute and wait .
6-You'll find easylogging.txt file which provide you with the pipename, installation path,return code, timestamp .
 
 




