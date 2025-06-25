# Sisop-FP-2025-IT-A04

# Final Project Sistem Operasi IT

## Peraturan
1. Waktu pengerjaan dimulai hari Kamis (19 Juni 2025) setelah soal dibagikan hingga hari Rabu (25 Juni 2025) pukul 23.59 WIB.
2. Praktikan diharapkan membuat laporan penjelasan dan penyelesaian soal dalam bentuk Readme(github).
3. Format nama repository github "Sisop-FP-2025-IT-[Kelas][Kelompok]" (contoh:Sisop-FP-2025-IT-A01).
4. Setelah pengerjaan selesai, seluruh source code dan semua script bash, awk, dan file yang berisi cron job ditaruh di github masing - masing kelompok, dan link github dikumpulkan pada form yang disediakan. Pastikan github di setting ke publik.
5. Commit terakhir maksimal 10 menit setelah waktu pengerjaan berakhir. Jika melewati maka akan dinilai berdasarkan commit terakhir.
6. Jika tidak ada pengumuman perubahan soal oleh asisten, maka soal dianggap dapat diselesaikan.
7. Jika ditemukan soal yang tidak dapat diselesaikan, harap menuliskannya pada Readme beserta permasalahan yang ditemukan.
8. Praktikan tidak diperbolehkan menanyakan jawaban dari soal yang diberikan kepada asisten maupun praktikan dari kelompok lainnya.
9. Jika ditemukan indikasi kecurangan dalam bentuk apapun di pengerjaan soal final project, maka nilai dianggap 0.
10. Pengerjaan soal final project sesuai dengan modul yang telah diajarkan.

## Kelompok IT-XX

Nama | NRP
--- | ---
Oscaryavat Viryavan | 5027241053
Evan Christian Nainggolan | 5027241026
Mey Rosalina | 5027241004
Mutiara Diva Jaladita | 5027241083

## Deskripsi Soal

**Zombie Cleaner Daemon (8)**

Buatlah program daemon yang pada saat start langsung membuat beberapa child process (misal 3–5) yang masing-masing melakukan tugas sederhana seperti sleep selama beberapa detik, lalu keluar. Setelah itu, daemon utama harus secara periodik (setiap 1 detik) memantau child process yang telah selesai namun belum di-wait (zombie), menggunakan fungsi waitpid dengan opsi non-blocking. Setiap kali menemukan dan membersihkan zombie, program harus mencatat PID dan status keluar child tersebut (exit code) ke file log atau terminal, sehingga terlihat bahwa zombie process berhasil dideteksi dan dihapus oleh daemon.

### Catatan

Struktur repository:
```
.
├── zombie_cleaner.c          # Program utama daemon zombie cleaner
├── README.md                 # Dokumentasi lengkap
└── zombie_cleaner.log        # File log yang dihasilkan program (auto-generated)
```

## Pengerjaan

### **1. Pemahaman Konsep Zombie Process**

**Teori**

Zombie process adalah proses yang telah selesai eksekusi tetapi masih memiliki entry di process table. Hal ini terjadi karena:

1. **Child process** telah selesai eksekusi dan memanggil `exit()`
2. **Parent process** belum memanggil `wait()` atau `waitpid()` untuk membaca exit status
3. Kernel tetap menyimpan informasi minimal tentang child (PID, exit status) sampai parent membacanya

**Masalah yang ditimbulkan:**
- Menghabiskan slot di process table
- Jika terlalu banyak, bisa menyebabkan system kehabisan PID
- Memory leak dalam bentuk process table entries

**Solusi**

Program daemon zombie cleaner mengatasi masalah ini dengan:

```c
// Menggunakan waitpid dengan WNOHANG untuk non-blocking wait
while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    // Membersihkan zombie dan mencatat informasinya
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        // Log PID dan exit code
    }
}
```

### **2. Implementasi Daemon Process**

**Teori**

Daemon adalah background process yang berjalan terus-menerus tanpa interaksi langsung dengan user. Karakteristik daemon:

1. **Detached dari terminal** - tidak terikat dengan session terminal
2. **Background execution** - berjalan di background
3. **Persistent** - berjalan sampai system shutdown atau dihentikan manual
4. **Signal handling** - menangani signal untuk graceful shutdown

**Solusi**

```c
// Signal handler untuk graceful shutdown
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
        log_message("Daemon received shutdown signal");
    }
}

// Setup signal handlers
signal(SIGINT, signal_handler);   // Ctrl+C
signal(SIGTERM, signal_handler);  // Termination signal
```

### **3. Child Process Creation dan Management**

**Teori**

Program menggunakan `fork()` system call untuk membuat child processes:

1. **fork()** membuat copy exact dari parent process
2. Return value berbeda untuk parent dan child:
   - Parent: mendapat PID child
   - Child: mendapat 0
   - Error: mendapat -1

**Solusi**

```c
void create_children(void) {
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process - melakukan tugas dan exit
            child_process(i + 1);
        } else if (pid > 0) {
            // Parent process - simpan PID child
            child_pids[i] = pid;
            num_active_children++;
        } else {
            // Fork failed
            log_message("ERROR: Fork failed");
        }
    }
}
```

### **4. Non-blocking Zombie Detection**

**Teori**

`waitpid()` dengan opsi `WNOHANG` memungkinkan parent process untuk:

1. **Non-blocking check** - tidak menunggu jika tidak ada child yang selesai
2. **Immediate return** - return 0 jika tidak ada zombie
3. **Multiple zombie handling** - bisa membersihkan beberapa zombie sekaligus

**Solusi**

```c
void clean_zombies(void) {
    pid_t pid;
    int status;
    
    // Loop sampai tidak ada lagi zombie yang bisa dibersihkan
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        total_zombies_cleaned++;
        
        // Analisis cara child process terminate
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("ZOMBIE CLEANED: PID %d exited with code %d\n", pid, exit_code);
        } else if (WIFSIGNALED(status)) {
            int signal_num = WTERMSIG(status);
            printf("ZOMBIE CLEANED: PID %d killed by signal %d\n", pid, signal_num);
        }
        
        num_active_children--;
    }
}
```

### **5. Logging dan Monitoring**

**Teori**

Logging penting untuk:

1. **Debugging** - melacak behavior program
2. **Monitoring** - memantau performa daemon
3. **Audit trail** - record aktivitas system

**Solusi**

```c
void log_message(const char *message) {
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    
    // Print ke terminal dengan color coding
    if (strstr(message, "ZOMBIE CLEANED")) {
        printf("\033[1;32m[%s] %s\033[0m\n", timestamp, message); // Green
    }
    
    // Write ke log file
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", timestamp, message);
        fclose(log_file);
    }
}
```

### **6. Cara Kompilasi dan Menjalankan**

```bash
# Kompilasi program
gcc -Wall -Wextra -std=c99 -o zombie_cleaner zombie_cleaner.c

# Jalankan program
./zombie_cleaner

# Untuk melihat help
./zombie_cleaner --help

# Monitor proses di terminal lain
watch -n 1 'ps aux | grep zombie_cleaner'

# Lihat zombie processes (jika ada)
ps aux | awk '$8 ~ /^Z/'
```

### **7. Output Program**

Program akan menghasilkan output seperti:

```
╔══════════════════════════════════════════════════════════════╗
║                    ZOMBIE CLEANER DAEMON                    ║
║                Final Project Sistem Operasi                 ║
╚══════════════════════════════════════════════════════════════╝

[2024-01-15 10:30:01] === ZOMBIE CLEANER DAEMON STARTED ===
[2024-01-15 10:30:01] Daemon PID: 12345
[2024-01-15 10:30:01] Creating child processes...
[2024-01-15 10:30:01] Created child 1 with PID: 12346
[2024-01-15 10:30:01] Child 1 (PID: 12346) started
[2024-01-15 10:30:01] Child 1 (PID: 12346) working for 5 seconds
...
[2024-01-15 10:30:06] ZOMBIE CLEANED: PID 12346 exited normally with exit code 1
[2024-01-15 10:30:06] Cleaned 1 zombie(s) in this cycle
[2024-01-15 10:30:06] STATS: Active children: 4, Total zombies cleaned: 1
```

### **8. Fitur Program**

✅ **Daemon functionality** - Berjalan sebagai background process  
✅ **Child process creation** - Membuat 5 child processes  
✅ **Zombie detection** - Menggunakan waitpid() dengan WNOHANG  
✅ **Non-blocking monitoring** - Check setiap 1 detik tanpa blocking  
✅ **Comprehensive logging** - Log ke terminal dan file dengan timestamp  
✅ **Signal handling** - Graceful shutdown dengan Ctrl+C  
✅ **Exit status analysis** - Mencatat PID dan exit code detail  
✅ **Statistics tracking** - Monitor jumlah zombie yang dibersihkan  
✅ **Color-coded output** - Output terminal dengan warna untuk readability  
✅ **Error handling** - Proper error handling untuk system calls  

**Video Menjalankan Program**

[Link video demonstrasi program - akan diupload setelah testing]

## Daftar Pustaka

1. Stevens, W. Richard, and Stephen A. Rago. "Advanced Programming in the UNIX Environment, Third Edition." Addison-Wesley Professional, 2013.

2. Kerrisk, Michael. "The Linux Programming Interface: A Linux and UNIX System Programming Handbook." No Starch Press, 2010.

3. Love, Robert. "Linux System Programming: Talking Directly to the Kernel and C Library." O'Reilly Media, 2013.

4. Linux Manual Pages. "waitpid(2) - Linux manual page." https://man7.org/linux/man-pages/man2/waitpid.2.html

5. Linux Manual Pages. "fork(2) - Linux manual page." https://man7.org/linux/man-pages/man2/fork.2.html
