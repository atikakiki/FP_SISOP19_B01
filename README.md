# FP_SISOP19_B01

Atika Rizki Nurakhmah (05111740000015)
Muhammad Izzudin (05111740000035)

### Soal
Buatlah sebuah music player dengan bahasa C yang memiliki fitur play nama_lagu, pause, next, prev, list lagu. 
Selain music player juga terdapat FUSE untuk mengumpulkan semua jenis file yang berekstensi .mp3 kedalam FUSE yang tersebar pada direktori /home/user. 
Ketika FUSE dijalankan, direktori hasil FUSE hanya berisi file .mp3 tanpa ada direktori lain di dalamnya. 
Asal file tersebut bisa tersebar dari berbagai folder dan subfolder. program mp3 mengarah ke FUSE untuk memutar musik.
Note: playlist bisa banyak, link mp3player

### Penyelesaian
Pertama gunakan fuse untuk memfilter folder .mp3
