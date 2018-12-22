/**
 *  @title      :   sr_container.c
 *  @author     :   Shabir Abdul Samadh (shabirmean@cs.mcgill.ca)
 *  @date       :   20th Nov 2018
 *  @purpose    :   COMP310/ECSE427 Operating Systems (Assingment 3) - Phase 2
 *  @description:   A template C code to be filled in order to spawn container instances
 *  @compilation:   Use "make container" with the given Makefile
*/

#include "sr_container.h"

/**
 *  The cgroup setting to add the writing task to the cgroup
 *  '0' is considered a special value and writing it to 'tasks' asks for the wrinting
 *      process to be added to the cgroup.
 *  You must add this to all the controls you create so that it is added to the task list.
 *  See the example 'cgroups_control' added to the array of controls - 'cgroups' - below
 **/
struct cgroup_setting self_to_task = {
	.name = "tasks",
	.value = "0"
};

typedef struct cgroup_setting CGROUP_SETTING;
/**
 *  ------------------------ TODO ------------------------
 *  An array of different cgroup-controllers.
 *  One controller has been been added for you.
 *  You should fill this array with the additional controls from commandline flags as described
 *      in the comments for the main() below
 *  ------------------------------------------------------
 **/
struct cgroups_control *cgroups[6] = {
	& (struct cgroups_control) {
		.control = CGRP_BLKIO_CONTROL,
		.settings = (struct cgroup_setting *[]) {
			& (struct cgroup_setting) {
				.name = "blkio.weight",
				.value = "64"
			},
			&self_to_task,             // must be added to all the new controls added
			NULL                       // NULL at the end of the array
		}
	},
	NULL                               // NULL at the end of the array
};

typedef struct cgroups_control CGROUPS_CONTROL;
/**
 *  ------------------------ TODO ------------------------
 *  The SRContainer by default suppoprts three flags:
 *          1. m : The rootfs of the container
 *          2. u : The userid mapping of the current user inside the container
 *          3. c : The initial process to run inside the container
 *
 *   You must extend it to support the following flags:
 *          1. C : The cpu shares weight to be set (cpu-cgroup controller)
 *          2. s : The cpu cores to which the container must be restricted (cpuset-cgroup controller)
 *          3. p : The max number of process's allowed within a container (pid-cgroup controller)
 *          4. M : The memory consumption allowed in the container (memory-cgroup controller)
 *          5. r : The read IO rate in bytes (blkio-cgroup controller)
 *          6. w : The write IO rate in bytes (blkio-cgroup controller)
 *          7. H : The hostname of the container
 *
 *   You can follow the current method followed to take in these flags and extend it.
 *   Note that the current implementation necessitates the "-c" flag to be the last one.
 *   For flags 1-6 you can add a new 'cgroups_control' to the existing 'cgroups' array
 *   For 7 you have to just set the hostname parameter of the 'child_config' struct in the header file
 *  ------------------------------------------------------
 **/
int main(int argc, char **argv)
{
    int i=0;
    struct child_config config = {0};
    int option = 0;
    int sockets[2] = {0};
    pid_t child_pid = 0;
    int last_optind = 0;
    int num_settings = 0;
    int realloc_size = 0;
    int shift_index = 0;
    bool found_cflag = false;
    char *temp_opt;

    CGROUPS_CONTROL* cpu_cgroup;
    CGROUPS_CONTROL *cpuset_cgroup;
    CGROUPS_CONTROL *pids_max_cgroup;
    CGROUPS_CONTROL *mem_cgroup;

    while ((option = getopt(argc, argv, "c:m:u:C:s:p:M:r:w:H:")))
    {
        if (found_cflag)
            break;

        switch (option)
        {
        case 'c':
            config.argc = argc - last_optind - 1;
            config.argv = &argv[argc - config.argc];
            found_cflag = true;
            break;

        case 'm':
            config.mount_dir = optarg;
            break;

        case 'u':
            if (sscanf(optarg, "%d", &config.uid) != 1)
            {
                fprintf(stderr, "UID not as expected: %s\n", optarg);
                cleanup_stuff(argv, sockets);
                return EXIT_FAILURE;
            }
            break;

        case 'C':;
            cpu_cgroup = (CGROUPS_CONTROL *) malloc(sizeof(CGROUPS_CONTROL));
            //set Control:
            strcpy(cpu_cgroup->control, CGRP_CPU_CONTROL);

			//set settings
            cpu_cgroup->settings = calloc(3, sizeof(CGROUP_SETTING *));

            cpu_cgroup->settings[0] = malloc(sizeof(CGROUP_SETTING));
            cpu_cgroup->settings[1] = malloc(sizeof(CGROUP_SETTING));
            cpu_cgroup->settings[2] = malloc(sizeof(CGROUP_SETTING));

            strcpy(cpu_cgroup->settings[0]->name, "cpu.shares");
            strcpy(cpu_cgroup->settings[0]->value, optarg);
			cpu_cgroup->settings[1] = &self_to_task;
			cpu_cgroup->settings[2] = NULL;

            //Add cgroup control to the array at the 1st null spot
            for (int i = 0; i < 6; i++)
            {
                if (cgroups[i] == NULL)
                {
                    cgroups[i] = cpu_cgroup;
                    if (i != 5)
                    {
                        i++; //make sure we don't go out of bounds
                        cgroups[i] = NULL; //set next spot to null
                    }
                    break;
                }
            }
            break;

        case 's':;
            //set Control:
            cpuset_cgroup = (CGROUPS_CONTROL *) malloc(sizeof(CGROUPS_CONTROL));
            strcpy(cpuset_cgroup->control, CGRP_CPU_SET_CONTROL);

            //set Settings:
            cpuset_cgroup->settings = calloc(4, sizeof(CGROUP_SETTING *));
        
            cpuset_cgroup->settings[0] = malloc(sizeof(CGROUP_SETTING));
            cpuset_cgroup->settings[1] = malloc(sizeof(CGROUP_SETTING));
            cpuset_cgroup->settings[2] = malloc(sizeof(CGROUP_SETTING));
            cpuset_cgroup->settings[3] = malloc(sizeof(CGROUP_SETTING));

            strcpy(cpuset_cgroup->settings[0]->name, "cpuset.cpus");
            strcpy(cpuset_cgroup->settings[0]->value, optarg);
            strcpy(cpuset_cgroup->settings[1]->name, "cpuset.mems");
            strcpy(cpuset_cgroup->settings[1]->value, "0-1");

            cpuset_cgroup->settings[2] = &self_to_task;
			cpuset_cgroup->settings[3] = NULL;

            //Add cgroup control to the array at the 1st null spot
            for (int i = 0; i < 6; i++)
            {
                if (cgroups[i] == NULL)
                {
                    cgroups[i] = cpuset_cgroup;
                    if (i != 5)
                    {
                        i++; //make sure we don't go out of bounds
                        cgroups[i] = NULL; //set next spot to null
                    }
                    break;
                }
            }
            break;

        case 'p':;
            //Note, min value is 2 to be able to run the container.
            pids_max_cgroup = (CGROUPS_CONTROL *) malloc(sizeof(CGROUPS_CONTROL));
            //set Control:
            strcpy(pids_max_cgroup->control, CGRP_PIDS_CONTROL);
            //set Settings:
            pids_max_cgroup->settings = calloc(3, sizeof(CGROUP_SETTING *));
            pids_max_cgroup->settings[0] = malloc(sizeof(CGROUP_SETTING)); 
            pids_max_cgroup->settings[1] = malloc(sizeof(CGROUP_SETTING));
            pids_max_cgroup->settings[2] = malloc(sizeof(CGROUP_SETTING));

            strcpy(pids_max_cgroup->settings[0]->name, "pids.max");
            strcpy(pids_max_cgroup->settings[0]->value, optarg);
            pids_max_cgroup->settings[1] = &self_to_task;
            pids_max_cgroup->settings[2] = NULL;

            //Add cgroup control to the array at the 1st null spot
            for (int i = 0; i < 6; i++)
            {
                    if (cgroups[i] == NULL)
                    {
                            cgroups[i] = pids_max_cgroup;
                            if (i != 5)
                            {
                                    i++; //make sure we don't go out of bounds
                                    cgroups[i] = NULL; //set next spot to null
                            }
                            break;
                    }
            }
            break;

        case 'M':;
            mem_cgroup = (CGROUPS_CONTROL *) malloc(sizeof(CGROUPS_CONTROL));
            //set Control:
            strcpy(mem_cgroup->control, CGRP_MEMORY_CONTROL);
            //set Settings:
            mem_cgroup->settings = calloc(3, sizeof(CGROUP_SETTING *));
            mem_cgroup->settings[0] = malloc(sizeof(CGROUP_SETTING));
            mem_cgroup->settings[1] = malloc(sizeof(CGROUP_SETTING));
            mem_cgroup->settings[2] = malloc(sizeof(CGROUP_SETTING));

            strcpy(mem_cgroup->settings[0]->name, "memory.limit_in_bytes");
            strcpy(mem_cgroup->settings[0]->value, optarg);
            mem_cgroup->settings[1] = &self_to_task;
            mem_cgroup->settings[2] = NULL;

            //Add cgroup control to the array at the 1st null spot
            for (int i = 0; i < 6; i++)
            {
                if (cgroups[i] == NULL)
                {
                    cgroups[i] = mem_cgroup;
                    if (i != 5)
                    {
                            i++; //make sure we don't go out of bounds
                            cgroups[i] = NULL; //set next spot to null
                    }
                    break;
                }
            }
            break;

        case 'r':;
             //if there are 2 block settings, new size is 5, else new size is 4.
            i=0;
            while(cgroups[0]->settings[i] != NULL) i++;
            num_settings = i-1;
            printf("num_settings: %d", num_settings);
            
            realloc_size = (num_settings > 1)? 5 : 4;
            shift_index = (num_settings > 1)? 3 : 2;

            if(num_settings > 1) {
                temp_opt = malloc(256);
                strcpy(temp_opt, cgroups[0]->settings[2]->value);
            }
            
            //Realloc and shift last two elements.
            cgroups[0]->settings = (CGROUP_SETTING **) calloc(realloc_size, sizeof(CGROUP_SETTING *));

            //Allocate space
            for(int i=0; i<realloc_size; i++){
                cgroups[0]->settings[i] = (CGROUP_SETTING *) malloc(sizeof(CGROUP_SETTING));
            }

            //Add new setting into settings array
            strcpy(cgroups[0]->settings[0]->name, "blkio.weight");
            strcpy(cgroups[0]->settings[0]->value, "64");
            cgroups[0]->settings[1] = &self_to_task;
            
            strcpy(cgroups[0]->settings[shift_index]->name, "blkio.throttle.read_bps_device");
            strcpy(cgroups[0]->settings[shift_index]->value, optarg);
            
            if(num_settings > 1){
                strcpy(cgroups[0]->settings[shift_index-1]->name, "blkio.throttle.write_bps_device");
                strcpy(cgroups[0]->settings[shift_index-1]->value, temp_opt);
            }

            cgroups[0]->settings[realloc_size -1] = NULL;
            break;

        case 'w':;
            //if there are 2 block settings, new size is 5, else new size is 4.
            i=0;
            for(int i=0; cgroups[0]->settings[i] != NULL; i++) i++;
            num_settings = i-1;

            if(num_settings > 1) {
                temp_opt = malloc(256);
                strcpy(temp_opt, cgroups[0]->settings[2]->value);
                printf("%s\n", temp_opt);
            }

            realloc_size = (num_settings > 1)? 5 : 4;
            shift_index = (num_settings > 1)? 3 : 2;
            
            //Realloc and shift last two elements.
            cgroups[0]->settings = calloc(realloc_size, sizeof(CGROUP_SETTING *));

            //Allocate space
            for(int i=0; i<realloc_size; i++){
                cgroups[0]->settings[i] = malloc(sizeof(CGROUP_SETTING));
            }

            //Add new setting into settings array
            strcpy(cgroups[0]->settings[0]->name, "blkio.weight");
            strcpy(cgroups[0]->settings[0]->value, "64");
            cgroups[0]->settings[1] = &self_to_task;
            
            strcpy(cgroups[0]->settings[shift_index]->name, "blkio.throttle.write_bps_device");
            strcpy(cgroups[0]->settings[shift_index]->value, optarg);

            cgroups[0]->settings[realloc_size -1] = NULL;

            //if a setting was already set, add it back
            if(num_settings > 1){
                strcpy(cgroups[0]->settings[shift_index-1]->name, "blkio.throttle.read_bps_device");
                strcpy(cgroups[0]->settings[shift_index-1]->value, temp_opt);
            }
            break;;

        case 'H':;
				config.hostname = optarg;
            break;
            
        default:
            cleanup_stuff(argv, sockets);
            return EXIT_FAILURE;
        }
        last_optind = optind;
    }

    if (!config.argc || !config.mount_dir){
        cleanup_stuff(argv, sockets);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "####### > Checking if the host Linux version is compatible...");
    struct utsname host = {0};
    if (uname(&host))
    {
        fprintf(stderr, "invocation to uname() failed: %m\n");
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    int major = -1;
    int minor = -1;
    if (sscanf(host.release, "%u.%u.", &major, &minor) != 2)
    {
        fprintf(stderr, "major minor version is unknown: %s\n", host.release);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    if (major != 4 || (minor < 7))
    {
        fprintf(stderr, "Linux version must be 4.7.x or minor version less than 7: %s\n", host.release);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    if (strcmp(ARCH_TYPE, host.machine))
    {
        fprintf(stderr, "architecture must be x86_64: %s\n", host.machine);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "%s on %s.\n", host.release, host.machine);

    if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets))
    {
        fprintf(stderr, "invocation to socketpair() failed: %m\n");
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC))
    {
        fprintf(stderr, "invocation to fcntl() failed: %m\n");
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    config.fd = sockets[1];

    /**
     * ------------------------ TODO ------------------------
     * This method here is creating the control groups using the 'cgroups' array
     * Make sure you have filled in this array with the correct values from the command line flags
     * Nothing to write here, just caution to ensure the array is filled
     * ------------------------------------------------------
     **/
    if (setup_cgroup_controls(&config, cgroups))
    {
        clean_child_structures(&config, cgroups, NULL);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }

    /**
     * ------------------------ TODO ------------------------
     * Setup a stack and create a new child process using the clone() system call
     * Ensure you have correct flags for the following namespaces:
     *      Network, Cgroup, PID, IPC, Mount, UTS (You don't need to add user namespace)
     * Set the return value of clone to 'child_pid'
     * Ensure to add 'SIGCHLD' flag to the clone() call
     * You can use the 'child_function' given below as the function to run in the cloned process
     * HINT: Note that the 'child_function' expects struct of type child_config.
     * ------------------------------------------------------
     **/

 
    void *stack = malloc(STACK_SIZE);
    if (stack == NULL)
        perror("stack malloc error");
    void *stack_top = stack + STACK_SIZE;  /* Assume stack grows downward */

    int flags = CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWCGROUP | SIGCHLD;

    child_pid = clone(child_function, stack_top, flags, &config);

    /**
     *  ------------------------------------------------------
     **/
    if (child_pid == -1)
    {
        fprintf(stderr, "####### > child creation failed! %m\n");
        clean_child_structures(&config, cgroups, stack);
        cleanup_sockets(sockets);
        return EXIT_FAILURE;
    }
    close(sockets[1]);
    sockets[1] = 0;

    if (setup_child_uid_map(child_pid, sockets[0]))
    {
        if (child_pid)
        kill(child_pid, SIGKILL);
    }

    int child_status = 0;
    waitpid(child_pid, &child_status, 0);
    int exit_status = WEXITSTATUS(child_status);

    clean_child_structures(&config, cgroups, stack);
    cleanup_sockets(sockets);
    return exit_status;
}


int child_function(void *arg)
{
    struct child_config *config = arg;
    if (sethostname(config->hostname, strlen(config->hostname)) || \
                setup_child_mounts(config) || \
                setup_child_userns(config) || \
                setup_child_capabilities() || \
                setup_syscall_filters()
        )
    {
        close(config->fd);
        return -1;
    }
    if (close(config->fd))
    {
        fprintf(stderr, "invocation to close() failed: %m\n");
        return -1;
    }
    if (execve(config->argv[0], config->argv, NULL))
    {
        perror("errno ");
        printf("%d", errno);
        fprintf(stderr, "invocation to execve() failed! %m.\n");
        return -1;
    }
    return 0;
}
