# Smart Water Meter: Sistem Monitoring dan Deteksi Anomali Air Rumah Tangga

## Deskripsi Proyek
Smart Water Meter adalah sistem IoT berbasis ESP32 yang dirancang untuk memantau konsumsi air rumah tangga secara real-time, mendeteksi kebocoran menggunakan model Extreme Learning Machine (ELM), serta menghitung estimasi tagihan air. Sistem ini menampilkan data melalui platform Blynk dan memberikan notifikasi kepada pengguna apabila terjadi anomali seperti kebocoran.

## Fitur Utama
- Monitoring konsumsi air secara real-time
- Estimasi biaya tagihan air berdasarkan tarif PDAM
- Deteksi kebocoran pipa menggunakan model AI
- Notifikasi otomatis ke aplikasi pengguna
- Sistem fault-tolerant melalui median voting antar sensor

## Perangkat Keras
- ESP32 sebagai mikrokontroler utama
- Sensor flow meter YF-S201 (9 unit: 3 titik x 3 sensor)
- Sensor tekanan WISNER WPT-83G-EGG4
- RTC DS3231 sebagai penanda waktu real-time
- Pipa dan keran simulasi untuk dapur dan kamar mandi

## Perangkat Lunak
- Arduino IDE untuk pengembangan firmware ESP32
- Python (Scikit-learn, NumPy) untuk pelatihan model ELM
- Blynk Cloud sebagai platform monitoring dan notifikasi
- WiFi sebagai protokol komunikasi

## Dataset dan Model AI
- Total data: 3.648 baris simulasi
- Fitur input: flow rate (main, dapur, kamar mandi) dan tekanan air
- Label: status kebocoran (main pipe, dapur, kamar mandi)
- Model: Extreme Learning Machine (ELM)
  - Arsitektur: 4 input, 240 hidden neuron, 3 output
  - Akurasi: sekitar 80%
- Model dikonversi ke file header (.h) dan di-embed ke ESP32


## Tampilan Dashboard Blynk
- Labeled display untuk flow rate (main, dapur, kamar mandi)
- Gauge tekanan air (psi)
- Estimasi biaya PDAM
- Grafik penggunaan air dari waktu ke waktu
- Input manual untuk tarif air per liter
- Push notification ketika terdeteksi kebocoran

## Notifikasi
- `leak_main_pipe`: Terjadi bocor di main pipe
- `leak_kitchen`: Terjadi bocor di kitchen
- `leak_bathroom`: Terjadi bocor di bathroom
- `system_normal`: Sistem normal, tidak ada kebocoran

## Pengujian Sistem
| Skenario                    | Langkah                              | Output yang Diharapkan           |
|----------------------------|------------------------------------------|----------------------------------|
| Semua kran tertutup        | Tidak ada aliran                         | Semua flow meter menunjukkan 0   |
| Bocor di pipa utama        | Flow main > 0, lainnya = 0               | Notifikasi bocor main pipe       |
| Bocor di dapur             | Flow dapur > 0 tanpa kran dibuka         | Notifikasi bocor dapur           |
| Bocor di kamar mandi       | Flow kamar mandi > 0 tanpa kran dibuka   | Sistem tidak mendeteksi bocoran  |
| Penggunaan air normal      | Aliran 2–3 LPM selama 5 menit            | Sistem tidak mendeteksi bocoran  |

## Cara Deploy Model ke ESP32
1. Jalankan `training_elm.py` untuk melatih model dan menghasilkan file `.npz`
2. Jalankan `convert_to_header.py` untuk mengubah model menjadi `elm_model.h`
3. Unggah semua file `.ino` dan `elm_model.h` ke ESP32 melalui Arduino IDE
4. Hubungkan ESP32 ke WiFi dan aplikasi Blynk

## Tim Pengembang
- Dzakwan Athallah P. – Teknik Informatika
- Manfredy Patarida H. M. – Teknik Komputer
- Erlinda Butarbutar – Teknik Komputer
- Angeline Indah N. K. – Teknik Komputer
- Fahrudin Bintang P. – Teknik Komputer

## Institusi
Fakultas Ilmu Komputer, Universitas Brawijaya  
Capstone Project 2025 – Mata Kuliah Embedded AI, Cloud Computing, Fault Tolerant System

