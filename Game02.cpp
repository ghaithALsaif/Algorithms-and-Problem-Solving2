#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>

using namespace std;

// --- هياكل البيانات ---
struct ResourceNode {
    string type;
    int amount;
};

struct Building {
    string name;
    string type; // زراعية، صناعية، سكنية، عسكرية
    string benefit;
};

struct Kingdom {
    string name;
    string biome;
    int relationship; // -100 عدو، 0 محايد، 100 حليف
    int militaryPower;
    int ironSupply;
    int woodSupply;
    int foodSupply;
};

struct Player {
    string name;
    string currentBiome = "الغابة";
    int health = 100;
    int gold = 200;
    int wood = 0;
    int iron = 0;
    int magicEssence = 0;
    int armySize = 5;
    int reputation = 0;
    vector<Building> kingdomBuildings;
    // جرعات سحرية
    bool hasFirePotion = false;
    bool hasInvisPotion = false;
};

// --- متغيرات اللعبة ---
Player player;
vector<Kingdom> worldKingdoms;
map<string, int> marketPrices;

// --- دوال مساعدة ---
void sleep(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}

void printText(const string& text) {
    for (char c : text) {
        cout << c << flush;
        sleep(10);
    }
    cout << endl;
}

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// --- نظام توليد العالم ---
void GenerateWorld() {
    worldKingdoms.push_back({"مملكة الجبال الشمالية", "الجبال", 0, 200, 500, 100, 150});
    worldKingdoms.push_back({"إمبراطورية الرمال", "الصحراء", 0, 250, 50, 300, 100});
    worldKingdoms.push_back({"تحالف الغابة القديمة", "الغابة", 0, 150, 150, 500, 200});
    marketPrices["Iron"] = 50;
    marketPrices["Wood"] = 20;
}

// --- تحديث الاقتصاد الديناميكي ---
void UpdateEconomy() {
    for (auto& k : worldKingdoms) {
        // كلما زاد العرض، قل السعر
        marketPrices["Iron"] = 1000 / (k.ironSupply / 10 + 1);
        marketPrices["Wood"] = 1000 / (k.woodSupply / 10 + 1);
    }
}

// --- واجهة المستخدم الرئيسية ---
void DisplayUI() {
    cout << "\n==================================================\n";
    cout << " الموقع: " << player.currentBiome << " | الصحة: " << player.health << "%\n";
    cout << " الذهب: " << player.gold << " | خشب: " << player.wood << " | حديد: " << player.iron << endl;
    cout << " الجيش: " << player.armySize << " جندي | السمعة: " << player.reputation << endl;
    cout << " مملكة اللاعب: " << player.kingdomBuildings.size() << " مباني\n";
    if(player.hasFirePotion) cout << " [لديك جرعة مقاومة النار]\n";
    if(player.hasInvisPotion) cout << " [لديك جرعة التخفي]\n";
    cout << "==================================================\n";
}

// --- نظام التنقل ---
void Travel() {
    cout << "\nأين تريد أن تذهب؟\n";
    cout << "1. الجبال (بيئة قاسية، غنية بالحديد)\n";
    cout << "2. الصحراء (بيئة تجارية، فقيرة بالموارد)\n";
    cout << "3. الغابة (بيئة معتدلة، غنية بالخشب)\n";
    cout << "4. أراضي مملكتك\n";
    cout << "5. الوجه الآخر للعالم (عالم سحري خفي)\n";
    int choice;
    cin >> choice;
    
    if(choice == 1) player.currentBiome = "الجبال";
    else if(choice == 2) player.currentBiome = "الصحراء";
    else if(choice == 3) player.currentBiome = "الغابة";
    else if(choice == 4) player.currentBiome = "أراضي مملكتك";
    else if(choice == 5) player.currentBiome = "الوجه الآخر";
    
    printText("* تسافر إلى " + player.currentBiome + "...");
}

// --- نظام جمع الموارد (تشبه تكسير مكعبات ماين كرافت) ---
void GatherResources() {
    printText("\n* تبدأ بتكسير الأرض وجمع الموارد...");
    if(player.currentBiome == "الجبال") {
        player.iron += 15; player.gold -= 5;
        printText("وجدت 15 حديد! (-5 ذهب للمجهود)");
    } else if(player.currentBiome == "الغابة") {
        player.wood += 25; player.gold -= 5;
        printText("قطعت 25 خشب! (-5 ذهب للمجهود)");
    } else if(player.currentBiome == "الصحراء") {
        player.gold += 40;
        printText("بحثت في الرمال ووجدت 40 ذهب!");
    } else if(player.currentBiome == "أراضي مملكتك") {
        printText("لا توجد موارد برية هنا، استخدم مبانيك للإنتاج.");
    } else if(player.currentBiome == "الوجه الآخر") {
        printText("المكان خطر جداً، لا يمكنك جمع الموارد هنا بسلام!");
    }
}

// --- نظام بناء المملكة وتخصيص الأراضي ---
void BuildKingdom() {
    if(player.currentBiome != "أراضي مملكتك") {
        printText("يجب أن تكون في أراضي مملكتك لتتمكن من البناء!");
        return;
    }
    
    cout << "\n--- نظام البناء (خصخصة الأراضي) ---\n";
    cout << "1. منطقة زراعية (50 خشب) - تنتج ذهباً يومياً عبر المزارعين.\n";
    cout << "2. منطقة صناعية (50 حديد) - يبني الـNPCs متاجرهم وتزيد الموارد.\n";
    cout << "3. منطقة سكنية (30 خشب، 20 حديد) - يجذب السكان ويزيد الجيش.\n";
    cout << "4. ثكنة عسكرية (100 خشب، 50 حديد) - تجند 20 جندي.\n";
    int choice; cin >> choice;
    
    if(choice == 1 && player.wood >= 50) {
        player.wood -= 50; player.kingdomBuildings.push_back({"مزرعة", "زراعية", "+50 ذهب كل دور"});
        printText("* تم تخصيص أرض زراعية. بدأ المزارعون بالعمل تلقائياً! *");
    } else if(choice == 2 && player.iron >= 50) {
        player.iron -= 50; player.kingdomBuildings.push_back({"ورشة", "صناعية", "+10 حديد كل دور"});
        printText("* تم تخصيص أرض صناعية. بنى الحدادون متاجرهم. *");
    } else if(choice == 3 && player.wood >= 30 && player.iron >= 20) {
        player.wood -= 30; player.iron -= 20; player.kingdomBuildings.push_back({"بيوت سكنية", "سكنية", "+5 جيش كل دور"});
        printText("* تم بناء منازل. تدفق الـNPCs إليك. *");
    } else if(choice == 4 && player.wood >= 100 && player.iron >= 50) {
        player.wood -= 100; player.iron -= 50; player.armySize += 20;
        player.kingdomBuildings.push_back({"ثكنة", "عسكرية", "+20 جيش"});
        printText("* تم بناء الثكنة العسكرية. جيشك ازداد! *");
    } else {
        printText("موارد غير كافية لاختيارك.");
    }
}

// --- دورة المملكة (إنتاج الموارد) ---
void KingdomTick() {
    for(auto& b : player.kingdomBuildings) {
        if(b.type == "زراعية") player.gold += 50;
        if(b.type == "صناعية") player.iron += 10;
        if(b.type == "سكنية") player.armySize += 5;
    }
}

// --- نظام التجارة والاقتصاد ---
void Trade() {
    cout << "\nأي مملكة تريد التجارة معها؟\n";
    for(size_t i = 0; i < worldKingdoms.size(); i++) {
        cout << i+1 << ". " << worldKingdoms[i].name << " (البيئة: " << worldKingdoms[i].biome << ")\n";
    }
    int c; cin >> c;
    if(c > 0 && c <= worldKingdoms.size()) {
        Kingdom& k = worldKingdoms[c-1];
        UpdateEconomy();
        int ironP = 1000 / (k.ironSupply / 10 + 1);
        int woodP = 1000 / (k.woodSupply / 10 + 1);
        
        cout << "\nأسعار " << k.name << " -> الحديد: " << ironP << " | الخشب: " << woodP << endl;
        cout << "1. شراء حديد (10)\n2. بيع حديد (10)\n3. شراء خشب (10)\n4. بيع خشب (10)\n";
        int tc; cin >> tc;
        
        if(tc == 1 && player.gold >= ironP*10) { player.gold -= ironP*10; player.iron += 10; k.ironSupply -= 10; printText("تم الشراء."); }
        else if(tc == 2 && player.iron >= 10) { player.gold += ironP*10; player.iron -= 10; k.ironSupply += 10; printText("تم البيع."); }
        else if(tc == 3 && player.gold >= woodP*10) { player.gold -= woodP*10; player.wood += 10; k.woodSupply -= 10; printText("تم الشراء."); }
        else if(tc == 4 && player.wood >= 10) { player.gold += woodP*10; player.wood -= 10; k.woodSupply += 10; printText("تم البيع."); }
    }
}

// --- نظام الدبلوماسية والحرب ---
void Diplomacy() {
    cout << "\n--- الدبلوماسية والحروب ---\n";
    for(size_t i = 0; i < worldKingdoms.size(); i++) {
        cout << i+1 << ". " << worldKingdoms[i].name << " (العلاقة: " << worldKingdoms[i].relationship << " | جيشهم: " << worldKingdoms[i].militaryPower << ")\n";
    }
    cout << "اختر المملكة: ";
    int c; cin >> c;
    if(c > 0 && c <= worldKingdoms.size()) {
        Kingdom& k = worldKingdoms[c-1];
        cout << "1. إعطاء هدية (50 ذهب) لتحسين العلاقة.\n";
        cout << "2. إعلان الحرب ونهب القرى!\n";
        int action; cin >> action;
        if(action == 1 && player.gold >= 50) {
            player.gold -= 50; k.relationship += 20; player.reputation += 10;
            printText("* تحسنت علاقتك مع " + k.name + " *");
        } else if(action == 2) {
            printText("* هاجمت قرى " + k.name + "! بدأ القتال... *");
            if(player.armySize > k.militaryPower) {
                player.gold += 200; player.iron += 50;
                k.relationship = -100; player.reputation -= 30;
                k.militaryPower -= 50;
                printText("انتصرت! نهبت 200 ذهب و 50 حديد. أصبحت هذه المملكة عدوتك اللدود.");
            } else {
                player.armySize -= 20; player.health -= 30;
                k.relationship = -100;
                printText("خسرت المعركة! جيشك تكبد خسائر فادحة وصحتك تضررت.");
            }
        }
    }
}

// --- الوجه الآخر للعالم (السحر والوحوش) ---
void MagicRealm() {
    if(player.currentBiome != "الوجه الآخر") {
        printText("يجب أن تسافر إلى الوجه الآخر أولاً!");
        return;
    }
    
    cout << "\n--- الوجه الآخر (عالم السحر) ---\n";
    cout << "1. قتال وحش أسطوري (مكافأة: جوهر سحري).\n";
    cout << "2. زيارة ساحر غامض (تداول الجوهر السحري بجرعات).\n";
    int c; cin >> c;
    
    if(c == 1) {
        printText("* يظهر وحش مرعب من عالم آخر... *");
        if(player.hasInvisPotion) {
            printText("استخدمت جرعة التخفي! تجاوزت الوحش وسرقت 3 جوهر سحري.");
            player.magicEssence += 3;
            player.hasInvisPotion = false;
        } else if(player.armySize > 30) {
            player.magicEssence += 3;
            printText("انتصر جيشك بصعوبة! حصلت على 3 جوهر سحري.");
        } else {
            player.health -= 40; player.armySize -= 10;
            printText("هُزمت! الوحش ألحق بك أضراراً جسيمة.");
        }
    } else if(c == 2) {
        cout << "الساحر: أحتاج 3 جوهر سحري لصنع جرعة.\n";
        cout << "1. جرعة التخفي (تساعدك في السطو والهروب).\n";
        cout << "2. جرعة مقاومة النار (تحميك في المعارك القادمة).\n";
        int p; cin >> p;
        if(player.magicEssence >= 3) {
            player.magicEssence -= 3;
            if(p == 1) { player.hasInvisPotion = true; printText("حصلت على جرعة التخفي!"); }
            else if(p == 2) { player.hasFirePotion = true; printText("حصلت على جرعة مقاومة النار!"); }
        } else {
            printText("لا تملك جوهر سحري كافٍ!");
        }
    }
}

// --- الحلقة الرئيسية ---
int main() {
    srand(time(0));
    GenerateWorld();
    
    clearScreen();
    printText("==================================================");
    printText("   عوالم الفوكسل: حروب الممالك (النسخة الكاملة)    ");
    printText("==================================================");
    printText("أنت تستيقظ في عالم شاسع. لديك أحلام في بناء إمبراطورية...");
    cout << "أدخل اسم حاكمك: ";
    cin >> player.name;
    
    bool playing = true;
    while(playing) {
        KingdomTick(); // إنتاج الموارد من المباني
        DisplayUI();
        
        cout << "1. التنقل في العالم\n";
        cout << "2. تكسير وجمع الموارد\n";
        cout << "3. بناء وتخصيص أراضي المملكة\n";
        cout << "4. التجارة (اقتصاد ديناميكي)\n";
        cout << "5. الدبلوماسية والحروب\n";
        cout << "6. استكشاف الوجه الآخر (السحر)\n";
        cout << "7. إنهاء اللعبة\n";
        cout << "اختيارك: ";
        
        int choice;
        cin >> choice;
        
        switch(choice) {
            case 1: Travel(); break;
            case 2: GatherResources(); break;
            case 3: BuildKingdom(); break;
            case 4: Trade(); break;
            case 5: Diplomacy(); break;
            case 6: MagicRealm(); break;
            case 7: playing = false; printText("وداعاً أيها الحاكم " + player.name + "..."); break;
            default: cout << "اختيار خاطئ.\n";
        }
        
        if(player.health <= 0) {
            printText("لقد قتلت! انتهت اللعبة.");
            playing = false;
        }
    }
    
    return 0;
}