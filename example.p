#include <console>
#include <string>
#include <threads>

forward test_function(thread_id);

public test_function(thread_id)
{
	printf("Thread %d start\n", thread_id);
	
	new i = 0;
	
	while(1)
	{
		
		if(i > 100000000)
			break;
		
		i += 1;
	}
	
	printf("Thread %d end\n", thread_id);
	
	return 1;
}

main()
{
	print "Start program\n";
	
	new thread = create_thread("test_function");
	if(!thread)
	{
		printf("Create_thread error\n");
		return 1;
	}
	
	resume_thread(thread);
	
	for(new i = 0; i < 10; i++)
		print "Main thread\n";
	
	//destroy_thread(thread); // Crash
	
	wait_thread(thread);
	
	print "End program\n";
	
	return 0;
}