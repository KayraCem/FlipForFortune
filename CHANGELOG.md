# Flip for Fortune: Değişiklik Günlüğü

Bu dosya projeye hangi gün ne eklendiğini gösterir. En yeni gün en üstte.

---

## 24 Ekim 2026 (coinler)

### Düzeltmeler (2. tur)
- **Skull** yeniden: korsan kafatası (puantiyeli kırmızı bandana, altın diş, yeşil alevli gözler); tura: halatlı çapa. 3D: gözlerden **yeşil** alev, arkada çapraz korsan kılıçları.
- **Galaxy** eski coin şekline ve eski yüzlerine döndü (Saturn kaldırıldı).
- **Coin elde bakışı takip ediyor:** etrafa bakınca/dönünce avuçtaki coin (ve kanat, alev gibi efektleri) elle birlikte dönüyor (bütün coinlerde).
- **Renkli iniş patlaması** sadece Epic ve üstü coinlerde; daha toplu, yere yayılmıyor.
- **Pumpkin:** üstteki çukur yumuşak; sap ve yapraklar artık beyaz değil, koyu yeşil (aura materyali daha mat).
- **İkonlar** daha büyük (coin çerçeveyi dolduruyor) ve beyaza patlamıyor.

### Coin yenilemeleri
- **Skull:** sıfırdan yeni yüz (oyuk göz çukurları, elmacık, dişler, çatlak, rün halkası). Tura: çapraz kemikler ve mor hilal. Gözlerden 3D **mor alevler** yükseliyor, arkada çapraz kemikler.
- **Black Rose:** sıfırdan. Koyu kadife zemin, dikenli altın çerçeve, katmanlı kara gül ve dikenli sap. 3D gül (yumuşak yapraklar), kenarda dikenli sarmaşık, gülden dökülen yapraklar.
- **Calendar:** yeni yüz (kırmızı başlıklı EKİM sayfası, gün ızgarası, işaretli gün, kıvrık köşe, spiral halkalar). Tura: kum saati. Üstte 3D spiral cilt halkaları, sayfalar tek tek kalkıp kopuyor ve uçuyor.
- **Galaxy:** artık **Saturn** şeklinde: bantlı gezegen küresi ve eğik halka. Yörünge efektleri aynen duruyor.
- **Angel:** yeni kanat modeli (3 sıra yumuşak tüy, altın uçlar), kanatlar çırpıyor.
- **Phoenix:** alevler daha büyük ve yoğun. Efekt materyalleri artık şeffaf karışımlı, aydınlık zeminde de görünüyor.
- **Pumpkin:** sap coinin üstündeki çukura oturuyor (boşluk kalmadı), silindir sap, koyu yaprak ve kıvrık dal.
- **Parıltı (+) efekti** (Clover, Gold, Star, Ghost): sivri ışınlı 4 köşeli yıldızlar, hızlı parlayıp yavaş sönüyor, bazıları kenarda.

### Diğer
- **Eline düşme efektleri:** her coinde, coinin renginde şok dalgası halkası, yükselen yıldız parıltıları ve güçlü ışık flaşı. Toz ve yaprak efektleri büyüdü. Avuçtaki küçültme azaltıldı.
- **Coin ikonları:** UI'da artık her coinin 3/4 açıdan 3D çekilmiş görüntüsü (aurasıyla, şeffaf arka plan): Koleksiyon, Summon paneli, Enchant. Sahip olmadığın coinler karanlık siluet.
- Koleksiyonda TAK ve GERİ butonları en solda.

## 24 Ekim 2026

### UI sadeleştirme
- **Summon paneli:** başlık "Dilek Çeşmesi". Uzun liste yerine her nadirlik grubunda coinlerin kendi görünüşü ızgara halinde: sahip olduğun coinler renkli ve seviyeleri köşede, olmayanlar karanlık siluet ve "?". Grupta tek bir şans yüzdesi var, coinin adı ve tek tek şansı fareyle üzerine gelince görünüyor. Dağılım çubuğu ve "eksik para" yazısı kaldırıldı.
- **Yükseltmeler:** alt başlık, durum çipleri ve "AL:" yazısı kaldırıldı. Açıklamalar fareyle üzerine gelince görünüyor. Değer tek satırda ("+5% > +6%"), seviye sadece "Lv 3".
- **Enchant:** alt başlık, olası enchant tablosu ve alt not kaldırıldı. Coin kartında coinin kendi yüzü görünüyor, ok butonları küçüldü.
- Akan su artık kesik görünmüyor: yumuşak dalga, sabit kalınlık, suya giriş halkaları.

### Summon animasyonu
- Coinler artık çeşmenin ortasından değil, ön taraftaki havuz suyundan sıçrayarak çıkıyor; çoklu summonda yan yana farklı yerlerden.
- Summon başlarken çeşme çıkan nadirliğin rengiyle parlıyor, su kaynıyor (kıvılcım kabarcıkları).
- **Legendary ve üstü:** ekran parlıyor, büyük "LEGENDARY!" yazısı çıkıyor, coin ışık huzmesi içinde sudan yavaşça yükselip büyüyor, kamera yaklaşıyor, halka/alev patlamaları oluyor. Sonra coin kameraya geliyor.
- Su daha az şeffaf.

### Düzeltmeler (çeşme ve summon)
- Çeşme şeffaf ve beyaz görünüyordu: üçgenlerin yönü ters olduğu için iç yüzler çiziliyordu, renk bağlantısı da kopuktu. İkisi de düzeltildi; su, akan su ve rünler daha parlak.
- Sonuç butonlarının sırası: TAMAM solda, x10 en sağda.
- Summon sırasında sağ üstte paran görünüyor. Altta yanlışlıkla görünen HUD yazıları gizlendi.
- Çoklu summondaki "Yeni coin / En iyi" ve "Enter: Tamam" yazıları kaldırıldı.

### GitHub düzeltmesi
- Harita dosyası (CoinRoom.umap) karakter ve el modellerinin verisini de kaydettiği için 100 MB'ı aşıyordu, GitHub kabul etmiyordu. Artık bu veri haritaya yazılmıyor, modeller dosyalardan yükleniyor.
- github_upload.bat 95 MB üstü dosyaları önceden uyarıyor ve gönderilemeyen eski commitleri tek committe birleştiriyor.

### Summon sahnesi
- Summon'a basınca kamera ayrı bir **summon odasına** geçiyor: çeşmenin kopyası, karanlık mağara, taş kolonlar ve ruh alevleri. El ve HUD artık ekranı kapatmıyor.
- Sonuç ekranının altında **TEKRAR xN**, **SUMMON x10** (son summon x10 ise x1) ve **TAMAM** butonları var. Fiyat butonda yazıyor, para yetmezse buton kapalı. TEKRAR'da sahneden çıkılmıyor, TAMAM ile metroya dönülüyor.
- Boş yere tıklamak artık ekranı kapatmıyor (Enter / Esc = Tamam).

### Dilek Çeşmesi: "Ruh Kadehi"
- Yeni 3 katlı taş çeşme modeli: oymalı kolon, tırtıklı kaseler, altın şeritler ve çiviler, altın madalyonlar, havuz duvarında parlayan rünler. En tepede taş taç içinde ruh küresi var.
- Su animasyonu: havuz suyu halka halka dalgalanıyor ve ışıldıyor, kaselerden dökülen su akan ışık şeritleriyle aşağı iniyor, küre nabız gibi parlıyor.
- Havuzdaki coinler akıntıyla sallanıyor. Bazıları dipten yükselip suyun üstünde dönüyor ve geri batıyor. 2. katta da coinler var.

### Metro
- Peronun etrafına görünmez duvarlar eklendi: ray hattı, sol duvar, iki uç ve merdiven kapısı.

## 21 Ekim 2026

### Yürüme ve birinci şahıs kontrol
- **WASD** ile yürüme, **Shift** ile koşma. Karakter duvarlardan, kolonlardan ve çeşmeden geçmiyor, raylara düşmüyor.
- **Fare ile serbest bakış**: kafa her yöne dönüyor. Kollar bakışı takip ediyor, böylece el ve coin hep ekranda.
- Ekranın ortasında **nişangah** var. Baktığın eşya parlıyor ve adı yazıyor ("[E] Yükseltmeler"). **Sol tık / E** ile etkileşim, boş yere sol tık ile coin atılıyor.
- Bir panel açılınca veya **Alt** basılıyken fare imleci çıkıyor; HUD ikonlarına tıklanabiliyor.
- AutoFlip tuşu **A**'dan **F**'ye taşındı (A artık sola yürüme).
- Koleksiyon (C) ve Envanter (I) ikonlarında tuş rozeti sağ alt köşede.

### Dilek Çeşmesi
- Çeşmedeki sandık kaldırıldı. Dipte oyundaki gerçek coinlerden bir yığın var, birkaç coin suyun üstünde dönerek süzülüyor (ana menüdeki gibi).
- Çeşmeye tıklayınca Summon açılıyor, coinler sudan yükseliyor.

### Metro istasyonu (oda ve masa kaldırıldı)
- Küçük bir **metro istasyonu** eklendi (11 m peron): fayanslı duvarlar, beton zemin, sarı güvenlik şeridi, raylar, iki uçta karanlık tünel ve kırmızı sinyal lambaları.
- Yeşil çelik kolonlar, floresan lambalar (biri bozuk), banklar, çöp kutusu, "FORTUNA" istasyon tabelası, afişler ve sokağa çıkan **ÇIKIŞ** merdiveni var.
- **Dilek Çeşmesi = Summon**: sandık artık çeşmenin suyunun içinde, coinler sudan yükseliyor.
- **Evsizin köşesi = Yükseltmeler**: karton, battaniye, karton yazı ve varil ateşi. Evsiz modeli onaylanınca buraya gelecek.
- **Falcı masası = Enchant**: kristal küre, mumlar, perde. Büyücü kadın modeli onaylanınca buraya gelecek.
- Masadaki eşyalar (sandık, kumbara, örs, büyü kitabı) kaldırıldı; Koleksiyon ve Envanter HUD'daki ikonlardan açılıyor.
- Kamera 90° görüş açısıyla peronun ilerisine bakıyor.

### Ana karakter
- **Sokak genci** ana karakter eklendi:
  - Bere, eski ceket, kapüşon, atkı, parmaksız eldiven, yırtık kot.
  - Parçalı model: kafa, gövde, kollar, eller, bacaklar ayrı eklemlerden döner; animasyonlar kodla.
  - Nefes alma ve hafif ağırlık değiştirme hareketi var.
- Karakter masanın karşısında duruyor. Kamera masayı ve karakteri birlikte görecek şekilde ayarlandı.

### Coin atma
- Coin artık **karakterin avucundan** atılıyor ve **yine avucuna** düşüyor. İleride masa olmadan, yürürken de coin atılabilecek.
  - El önce aşağı çekilip yukarı fırlatıyor.
  - Coin havadayken karakter onu gözüyle takip ediyor.
  - Yakalarken el esneyip toparlanıyor.
- Eski yüzen el, karakter varken gizleniyor.
- Hazine sandığı ve masa lambası, karakterin önünü kapatmasın diye yana alındı.

### Birinci şahıs kamera
- Kamera artık karakterin gözünde (birinci şahıs) ve karaktere bağlı. Ekranda sadece kollar ve coin tutan el görünüyor, el biraz daha önde ve yukarıda duruyor.
- Karakterin ve elin simsiyah görünmesi düzeltildi: modellerin yüzey yönleri ters olduğu için ışık yanlış hesaplanıyordu.

### Sol menü
- Ekranın soluna ikonlu **Koleksiyon** (C tuşu) ve **Envanter** (I tuşu) butonları eklendi. Tıklayınca ya da tuşa basınca açılıp kapanıyor.

### Taslaklar (oyunda değil)
- Evsiz adam (Upgrade NPC) ilk taslak.

---

## 20 Ekim 2026

### Coinler
- **Lock Coin** baştan yapıldı:
  - Yazı yüzü kasa kapısı: 0–90 şifre kadranı, 12 altın cıvata, gösterge oku.
  - Ortada dönen 3 kollu 3D kasa tekerleği (`Vault` efekti).
  - Tura yüzünde zincir çerçeve içinde süslü eski anahtar.
- **Pumpkin Coin** artık balkabağı şeklinde: geniş gövde, dilimler, üstte sap çukuru. Yazıda oyulmuş yanan yüz, turada yanan yarasa.
- **Skull Coin** artık kafatası şeklinde: kafa, elmacık, dar çene. Kemik rengi yüz, mor alevli gözler, çatlak; arkada parlayan runlar ve çapraz 3D kemikler.

### El
- Coini atan el koyu deri eldiven oldu, başparmakta altın yüksük var.
- Elin içinin görünmesine yol açan ters yüzey hatası düzeltildi.

### Diğer
- `CHANGELOG.md` eklendi. `github_upload.bat` artık commit mesajını tarihli soruyor.

---

## 19 Ekim 2026

### Görsel tarz: Karanlık Fantezi
- Beş tarz denemesi yapıldı (antika, karanlık fantezi, sci-fi, lüks, low-poly). **Karanlık fantezi** seçildi.
- 18 coinin hepsi bu tarzda yeniden çizildi:
  - Ortak taban: koyu metal disk, kabartma metal amblem, parlayan runlar.
  - Coinler malzemeye (bronz, demir, gümüş, obsidyen, yeşim…) ve kenar süsüne (halat, cıvata, diken, sarmaşık, tüy, zincir…) göre ayrışıyor.

### Coin şekilleri ve 3D efektler
- **Yeni coin şekilleri:**
  - 12 köşe
  - Yıldız
  - Yonca
  - Hayalet
  - Ortası delik çark
- **Yeni 3D efektler:**
  - Alev (Phoenix)
  - Zincir ve asma kilit
  - Hale
  - Yörüngede gezegenler (Galaxy)
  - Işıltı
  - Dikenli sarmaşık ve siyah gül (Black Rose)
  - Yosun ve kabarcık (Seaweed)
  - Neon tehlike halkası (Zone)
  - Cıvatalar
  - Balkabağı sapı
  - Kanatlar
  - Saat akrep ve yelkovanı
  - Pusula iğnesi
  - Çapraz kemikler
- **Neon parlama:** coin yüzlerinde ışık saçan kısımlar nabız gibi parlıyor.
- **Yeni coinler:**
  - Seaweed (Rare)
  - Zone (Epic)
  - Black Rose (Legendary)
  - Lock (Legendary)

### Summon
- **Çoklu summon:** x1, x5, x10 (x10 = 1 bedava).
- **x10 açılımı:**
  - Sandık uzun sallanır, coinler sırayla fırlayıp ekranda diziliyor.
  - En iyi coin en son, ışık patlamasıyla geliyor.
  - Her coinin altında isim kartı var.
- Summon paneli yenilendi:
  - Koleksiyon ilerlemesi
  - Nadirlik dağılım çubuğu
  - Nadirliğe göre gruplu liste
  - Sahip olunan coinde "SAHİP" etiketi, olmayanda "YENİ" etiketi
- Summon sonrası artık Coin Collection'a atmıyor.

### Arayüz
- **Upgrade paneli:**
  - x1 / x10 / MAKS alım seçeneği
  - Seviye çubukları
  - Güncel değer etiketleri
  - Satın alınca satır parlıyor
- **Enchant paneli:**
  - Solda coin kartı
  - Tier şans çubukları
  - 13 boost'un tablosu (çark dönerken satırlar yanıyor)
- Paneller açılırken animasyonlu geliyor, kullanılabilir butonlar nabız gibi atıyor.
- **Ana menü:**
  - Düşen coinler artık oyundaki gerçek coinler.
  - Logo düzeltildi.
  - Sağ taraf yumuşak geçişli koyulaşıyor.
  - Para ve koleksiyon bilgisi var.

### El
- Coini fiskeyle atan el eklendi; sonra kürelerden yapılmış elin yerine pürüzsüz bir el modeli geldi.

### Test komutları
- `GiveAllCoins`, `GiveCoin <isim>`, `GiveMoney`, `GiveScrolls`.

---

## 18 Ekim 2026

### Masa ve oda
- **Masadaki eşyalar:**
  - Sandık: Inventory
  - Kumbara: Coin Collection
  - Büyü kitabı: Enchant
  - Örs: Upgrade
  - Hazine sandığı: Gacha / Summon
- Coin Collection için ayrı vitrin odası kuruldu.
- Ana menü için ayrı sahne kuruldu: sağda butonlar, solda yağan coinler.
- Oyun logosu yapıldı: "FFF: Flip For Fortune".

### Coinler
- Yeni coinler ve yeni nadirlikler eklendi (Mythic, Divine, Celestial, Secret).
- Coin modelleri: şekilli 3D coin (yuvarlak, sekizgen, altıgen, dilimli, dişli), yazı/tura dokuları, kabartma.
- Seviyeye göre değişen flip animasyonları eklendi.
- Coin masaya indiğinde inen efekt eklendi: toz, kıvılcım, altın halka, yaprak, alev.

### Gacha
- **Hazine sandığı:**
  - Sallanır, kapağı açılır, coin ekrana uçar.
  - "YENİ COIN!" ekranı çıkar.
  - Sandığa tıklayınca önce nadirlik ve şans listesi açılıyor.

### Arayüz
- Inventory UI: 6×4 eşya ızgarası ve detay paneli.
- Enchant UI: tier şansları, mevcut enchant, dönen çark.
- Upgrade paneli eklendi.

---

## 17 Ekim 2026

### Temel oyun
- Coin atma sistemi:
  - Yazı tam değer, tura yarım değer verir.
  - Kritik vuruş: 1.5x + CriticalMultiply.
- Yükseltmeler: CoinFlipSpeed, HeadChance, MoneyEarn, CriticalChance, CriticalMultiply.
- Coinler kullanıldıkça seviye atlar, seviye arttıkça değeri artar.
- Enchant Scroll (1/800 düşme şansı) ve 3 tier'lı enchant sistemi (13 boost).
- AutoFlip (500 elle atıştan sonra açılır).
- HUD: para, scroll, AUTO butonu, YAZI/TURA yazısı, seviye atlama yazısı.
- Coin Collection ekranı: açıklama, istatistik, seviye, enchant sayfaları.
- Kamera: sağ tıkla etrafa bakma, tekerlekle yakınlaşma.
- Proje GitHub'a bağlandı.

---

## 16 Ekim 2026

- Proje kuruldu (Unreal Engine 5.8, C++).
- Tasarım notları çıkarıldı; Visual Studio C++ araçları kuruldu.
