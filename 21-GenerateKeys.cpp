#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;



int RandomGenarator(int min, int max)
{
    return rand() % (max - min + 1) + min;
}

short readpositiveInteger(string massage)
{
    short number;
    do
    {
        cout << massage;
        cin >> number;
    }while (number <= 0);
    return number;
    
}


string GenerateKeys()
{
    string keys = "";
    for (short i = 0; i < 4; i++)
    {
        for (short j = 0; j < 4; j++)
        {
            keys += char(RandomGenarator(65, 90));
        }
        if (i < 3)
        {
            keys += "-";
        }
    }
    return keys;
}
void PrintKeys(short  order)
{
   for (short i = 0; i < order; i++)
   {
       cout <<"Key ["<< i + 1 << "]: " << GenerateKeys() << endl;
   }
}



int main()
{
    srand((unsigned)time(NULL));

PrintKeys(readpositiveInteger("Enter the order of the keys: "));
 return 0;
}



