#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;
enum enOrder
{
    
    SmallLetter=1,
    CapitalLetter=2,
    SpecialCharacter=3,
    Didit=4,
    Quit=5

};


int RandomGenarator(int min, int max)
{
    return rand() % (max - min + 1) + min;
}


enOrder ReadOrder()
{
    int number;
    cout << "Please select the order of the character you want to print: " << endl;
    cout << "1. Small Letter" << endl;
    cout << "2. Capital Letter" << endl;
    cout << "3. Special Character" << endl;
    cout << "4. Digit" << endl;
    cout << "5. Quit" << endl;
    cin >> number;
    return static_cast<enOrder>(number);
}

char RandomCharactergen(enOrder order)
{

    switch (order)
    {
    case enOrder::SmallLetter:
        return char(RandomGenarator(97, 122));
        break;
    case enOrder::CapitalLetter:
        return char(RandomGenarator(65, 90));
        break;
    case enOrder::SpecialCharacter:
        return char(RandomGenarator(33, 47));
        break;
    case enOrder::Didit:
        return char(RandomGenarator(48, 57));
        break;
    default:
        return ' ';
        break;
    }
}
void printRandomCharacter(enOrder order)
{
    char randomChar = RandomCharactergen(order);
    if (randomChar != ' ')
    {
        cout << "Random Character: " << randomChar << endl;
    }
    else
    {
        cout << "Invalid selection. Please try again." << endl;
    }
}


int main()
{
    srand((unsigned)time(NULL));

printRandomCharacter(ReadOrder());   
 return 0;
}



