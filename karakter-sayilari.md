# Oturum Boyunca Karakter Basina Soru Sayisi

**Kapsam:** kusursuz ogrenci (hic hata yok), varsayilan ayarlar
(Numbers acik, Symbols kapali), varsayilan pencere boyutu (640x240).

## 1. Oturumun sonu nasil tanimlandi

Program kendiliginden bitmiyor (`m.fl` icinde `while (1)`). Pratikte oturumu
bitiren sey, **tum harflerin cubugunun ekrandan kaybolmasi**. Esik su sekilde:

| Adim | Kaynak | Deger |
|---|---|---|
| Bargraph yuksekligi | `m.fl` → `Fl_Group Lesson xywh {0 25 640 185}` | 185 px |
| Slider widget yuksekligi | `Bargraph::tesselate` → `resize(..., h()-35)` | 150 px |
| Cizim alani | `Fl_Slider::draw` → `FL_DOWN_BOX` payi 4 px | **146 px** |
| Dolu piksel | `S = int(val*ww + .5)`, `min>max` oldugu icin `S = ww - S` | — |
| Kaybolma esigi | `S == 0` ⇔ `value <= 0.5/146` | **≈ 0.00342** |

Kusursuz ogrencide her dogru cevap `value *= 0.765625` (= `0.875²`, cunku
`overall` hep 0 kalir ve `grade()` guncellemeyi iki kez uygular).

`0.765625^n <= 0.00342` → **n = 22**. Yani her harf icin **22 dogru**.

Pencere buyudukce cubuk uzar, son piksel gec olur:

| Pencere | Cubuk | Gereken dogru |
|---|---:|---:|
| Varsayilan (640x240) | 146 px | 22 |
| Yariya buyutulmus | 300 px | 24 |
| Tam ekran 1080p | 986 px | 29 |

> Onceki surumde bitis noktasini "tum harfler terfi esigini (0.4) gecti" diye
> almistim. O kesme keyfiydi ve tabloyu bozuyordu — gec giren harfler yapay
> olarak dusuk gorunuyordu. Bu surum o hatayi duzeltir.

## 2. Oturum uzunlugu

- Ornek oturum (tohum 1756000000): **850 soru**
- 300 farkli tohum: ortalama **873.8 soru** (829–929)
- Teorik alt sinir: 36 x 22 = **792 soru**

## 3. Karakter basina soru sayisi

Sira = mufredata giris sirasi. `Ornek` = tek oturum, `Ort` = 300 oturum ortalamasi.

| Sira | Krk | Ornek | Ort | En az–En cok | Sira | Krk | Ornek | Ort | En az–En cok |
|---:|:---:|---:|---:|:---:|---:|:---:|---:|---:|:---:|
| 1 | `Q` | 25 | 24.3 | 22–29 | 19 | `X` | 22 | 24.3 | 22–29 |
| 2 | `7` | 23 | 24.3 | 22–29 | 20 | `D` | 23 | 24.1 | 22–28 |
| 3 | `Z` | 23 | 24.4 | 22–28 | 21 | `Y` | 22 | 24.2 | 22–28 |
| 4 | `G` | 23 | 24.4 | 22–29 | 22 | `C` | 24 | 24.1 | 22–28 |
| 5 | `0` | 28 | 24.3 | 22–30 | 23 | `K` | 22 | 24.2 | 22–28 |
| 6 | `9` | 22 | 24.4 | 22–29 | 24 | `N` | 23 | 24.3 | 22–28 |
| 7 | `8` | 23 | 24.4 | 22–29 | 25 | `2` | 26 | 24.3 | 22–28 |
| 8 | `O` | 23 | 24.4 | 22–29 | 26 | `3` | 23 | 24.3 | 22–27 |
| 9 | `1` | 26 | 24.2 | 22–30 | 27 | `F` | 25 | 24.1 | 22–29 |
| 10 | `J` | 25 | 24.4 | 22–29 | 28 | `U` | 24 | 24.2 | 22–28 |
| 11 | `P` | 23 | 24.2 | 22–28 | 29 | `4` | 24 | 24.2 | 22–29 |
| 12 | `W` | 24 | 24.3 | 22–28 | 30 | `5` | 22 | 24.1 | 22–30 |
| 13 | `L` | 23 | 24.4 | 22–29 | 31 | `V` | 24 | 24.2 | 22–28 |
| 14 | `R` | 23 | 24.5 | 22–29 | 32 | `H` | 22 | 24.2 | 22–28 |
| 15 | `A` | 23 | 24.6 | 22–29 | 33 | `S` | 22 | 24.1 | 22–29 |
| 16 | `M` | 22 | 24.5 | 22–29 | 34 | `I` | 25 | 24.1 | 22–29 |
| 17 | `6` | 27 | 24.4 | 22–29 | 35 | `T` | 22 | 24.1 | 22–28 |
| 18 | `B` | 24 | 24.4 | 22–29 | 36 | `E` | 25 | 24.1 | 22–28 |

**Ozet:**

- Ornek oturum: en az **22**, en cok **28**, ortalama **23.6**
- 300 oturum ortalamasinda karakterler arasi fark: **24.1 – 24.6**

| Grup | Adet | Ornek oturum | Ort/karakter |
|---|---:|---:|---:|
| Harf (A–Z) | 26 | 606 | 24.3 |
| Rakam (0–9) | 10 | 244 | 24.3 |
| **Toplam** | **36** | **850** | **24.3** |

## 4. Sonuc

**Tum karakterler pratikte esit sayida soruluyor.** Mufredattaki sirasi,
harf mi rakam mi oldugu, Morse kodunun uzunlugu — hicbiri toplam soru
sayisini degistirmiyor. Tek oturumdaki 22–28 sapmasi saf rastgelelik;
300 oturumda ortalamalar 24.1–24.6 bandina sikisiyor.

Sebep `Bargraph::select()`: cekilis hata oranina gore agirlikli, yani geride
kalan harf otomatik olarak daha sik geliyor. Sistem kendini dengeliyor.

Fark **zamanlamada**: yeni giren harf birkac soru boyunca yogun sorulur,
sonra seyrelir. Erken harfler ayni 22 doguruyu uzun bir sureye yayar,
gec harfler kisa surede toplar.
