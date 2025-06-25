/*
 * Zombie Cleaner Daemon
 * Final Project Sistem Operasi IT
 * 
 * Program daemon yang membuat child processes dan membersihkan zombie processes
 * secara periodik menggunakan waitpid() dengan opsi non-blocking.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define NUM_CHILDREN 5
#define LOG_FILE "zombie_cleaner.log"
#define MONITOR_INTERVAL 1  // 1 second
#define MAX_CYCLES 60       // Run for 60 seconds for demonstration

// Global variables
volatile int running = 1;
pid_t child_pids[NUM_CHILDREN];
int num_active_children = 0;
int total_zombies_cleaned = 0;

// Function prototypes
void get_timestamp(char *buffer, size_t size);
void log_message(const char *message);
void signal_handler(int sig);
void child_process(int child_id);
void create_children(void);
void clean_zombies(void);
void print_statistics(void);
void run_daemon(void);

/**
 * Get current timestamp in formatted string
 */
void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/**
 * Log message to both terminal and log file
 */
void log_message(const char *message) {
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    
    // Print to terminal with color coding
    if (strstr(message, "ZOMBIE CLEANED")) {
        printf("\033[1;32m[%s] %s\033[0m\n", timestamp, message); // Green
    } else if (strstr(message, "ERROR") || strstr(message, "FAILED")) {
        printf("\033[1;31m[%s] %s\033[0m\n", timestamp, message); // Red
    } else if (strstr(message, "Child")) {
        printf("\033[1;34m[%s] %s\033[0m\n", timestamp, message); // Blue
    } else {
        printf("[%s] %s\n", timestamp, message);
    }
    fflush(stdout);
    
    // Write to log file
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", timestamp, message);
        fclose(log_file);
    }
}

/**
 * Signal handler for graceful shutdown
 */
void signal_handler(int sig) {
    char msg[256];
    if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
        snprintf(msg, sizeof(msg), "Daemon received shutdown signal (%d)", sig);
        log_message(msg);
    }
}

/**
 * Child process function - simulates work then exits
 */
void child_process(int child_id) {
    char msg[256];
    
    snprintf(msg, sizeof(msg), "Child %d (PID: %d) started", child_id, getpid());
    log_message(msg);
    
    // Simulate some work with random sleep time
    srand(getpid() + time(NULL));
    int sleep_time = 2 + (rand() % 6); // Sleep 2-7 seconds
    
    snprintf(msg, sizeof(msg), "Child %d (PID: %d) working for %d seconds", 
             child_id, getpid(), sleep_time);
    log_message(msg);
    
    sleep(sleep_time);
    
    snprintf(msg, sizeof(msg), "Child %d (PID: %d) completed work", child_id, getpid());
    log_message(msg);
    
    // Exit with child_id as exit code for demonstration
    exit(child_id);
}

/**
 * Create multiple child processes
 */
void create_children(void) {
    char msg[256];
    
    log_message("Creating child processes...");
    
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process
            child_process(i + 1);
            // Never reaches here
        } else if (pid > 0) {
            // Parent process
            child_pids[i] = pid;
            num_active_children++;
            
            snprintf(msg, sizeof(msg), "Created child %d with PID: %d", i + 1, pid);
            log_message(msg);
        } else {
            // Fork failed
            snprintf(msg, sizeof(msg), "ERROR: Failed to create child %d: %s", 
                    i + 1, strerror(errno));
            log_message(msg);
        }
    }
    
    snprintf(msg, sizeof(msg), "Total children created: %d", num_active_children);
    log_message(msg);
}

/**
 * Check and clean zombie processes using waitpid with WNOHANG
 */
void clean_zombies(void) {
    pid_t pid;
    int status;
    char msg[256];
    int zombies_found = 0;
    
    // Use waitpid with WNOHANG for non-blocking wait
    // This prevents the parent from blocking while waiting for children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        zombies_found++;
        total_zombies_cleaned++;
        
        // Analyze how the child process terminated
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            snprintf(msg, sizeof(msg), 
                    "ZOMBIE CLEANED: PID %d exited normally with exit code %d", 
                    pid, exit_code);
        } else if (WIFSIGNALED(status)) {
            int signal_num = WTERMSIG(status);
            snprintf(msg, sizeof(msg), 
                    "ZOMBIE CLEANED: PID %d terminated by signal %d (%s)", 
                    pid, signal_num, strsignal(signal_num));
        } else if (WIFSTOPPED(status)) {
            int signal_num = WSTOPSIG(status);
            snprintf(msg, sizeof(msg), 
                    "ZOMBIE CLEANED: PID %d stopped by signal %d", 
                    pid, signal_num);
        } else {
            snprintf(msg, sizeof(msg), 
                    "ZOMBIE CLEANED: PID %d exited with unknown status (0x%x)", 
                    pid, status);
        }
        
        log_message(msg);
        num_active_children--;
    }
    
    // Check for waitpid errors (other than no children available)
    if (pid == -1 && errno != ECHILD && errno != EINTR) {
        snprintf(msg, sizeof(msg), "ERROR: waitpid failed: %s", strerror(errno));
        log_message(msg);
    }
    
    // Log if zombies were found in this cycle
    if (zombies_found > 0) {
        snprintf(msg, sizeof(msg), "Cleaned %d zombie(s) in this cycle", zombies_found);
        log_message(msg);
    }
}

/**
 * Print daemon statistics
 */
void print_statistics(void) {
    char msg[256];
    snprintf(msg, sizeof(msg), 
            "STATS: Active children: %d, Total zombies cleaned: %d", 
            num_active_children, total_zombies_cleaned);
    log_message(msg);
}

/**
 * Main daemon loop
 */
void run_daemon(void) {
    char msg[256];
    int cycle_count = 0;
    
    log_message("=== ZOMBIE CLEANER DAEMON STARTED ===");
    snprintf(msg, sizeof(msg), "Daemon PID: %d", getpid());
    log_message(msg);
    snprintf(msg, sizeof(msg), "Monitoring interval: %d second(s)", MONITOR_INTERVAL);
    log_message(msg);
    snprintf(msg, sizeof(msg), "Number of children to create: %d", NUM_CHILDREN);
    log_message(msg);
    
    // Create initial batch of child processes
    create_children();
    
    log_message("Starting zombie monitoring loop...");
    log_message("Press Ctrl+C to stop the daemon gracefully");
    
    // Main monitoring loop
    while (running && cycle_count < MAX_CYCLES) {
        cycle_count++;
        
        // Clean any zombie processes (non-blocking)
        clean_zombies();
        
        // Print periodic status every 5 seconds
        if (cycle_count % 5 == 0) {
            snprintf(msg, sizeof(msg), 
                    "Monitoring cycle %d - Checking for zombies...", cycle_count);
            log_message(msg);
            print_statistics();
        }
        
        // Create new batch of children if all are done (for demonstration)
        if (num_active_children == 0 && cycle_count < (MAX_CYCLES - 10)) {
            log_message("All children finished. Creating new batch for demonstration...");
            sleep(2); // Brief pause before creating new children
            create_children();
        }
        
        // Sleep for monitoring interval
        sleep(MONITOR_INTERVAL);
    }
    
    // Shutdown sequence
    if (cycle_count >= MAX_CYCLES) {
        log_message("Demonstration time limit reached. Shutting down...");
    }
    
    // Final cleanup - wait for any remaining children
    log_message("Performing final zombie cleanup...");
    int final_cleanup_attempts = 0;
    while (num_active_children > 0 && final_cleanup_attempts < 10) {
        clean_zombies();
        if (num_active_children > 0) {
            sleep(1);
            final_cleanup_attempts++;
        }
    }
    
    // Force kill any remaining children (shouldn't happen in normal operation)
    if (num_active_children > 0) {
        log_message("WARNING: Some children still active, sending SIGTERM...");
        for (int i = 0; i < NUM_CHILDREN; i++) {
            if (child_pids[i] > 0) {
                kill(child_pids[i], SIGTERM);
            }
        }
        sleep(2);
        clean_zombies();
    }
    
    // Final statistics
    print_statistics();
    log_message("=== ZOMBIE CLEANER DAEMON STOPPED ===");
}

/**
 * Main function
 */
int main(int argc, char *argv[]) {
    // Print program information
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    ZOMBIE CLEANER DAEMON                    ║\n");
    printf("║                Final Project Sistem Operasi                 ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Program Features:\n");
    printf("• Creates %d child processes that perform work and exit\n", NUM_CHILDREN);
    printf("• Monitors for zombie processes every %d second(s)\n", MONITOR_INTERVAL);
    printf("• Uses waitpid() with WNOHANG for non-blocking zombie cleanup\n");
    printf("• Logs all activities to terminal and '%s'\n", LOG_FILE);
    printf("• Handles signals for graceful shutdown\n\n");
    
    // Check for help argument
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        printf("Usage: %s [options]\n", argv[0]);
        printf("Options:\n");
        printf("  -h, --help    Show this help message\n");
        printf("\nTo compile: gcc -o zombie_cleaner zombie_cleaner.c\n");
        printf("To run: ./zombie_cleaner\n");
        return 0;
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // Termination signal
    
    // Initialize log file (clear previous contents)
    FILE *log_file = fopen(LOG_FILE, "w");
    if (log_file) {
        fprintf(log_file, "=== ZOMBIE CLEANER DAEMON LOG ===\n");
        fclose(log_file);
    } else {
        printf("WARNING: Could not create log file '%s'\n", LOG_FILE);
    }
    
    printf("Starting daemon... (Log file: %s)\n\n", LOG_FILE);
    
    // Run the main daemon
    run_daemon();
    
    printf("\nDaemon stopped. Check '%s' for complete log.\n", LOG_FILE);
    return 0;
}
