#include <iostream>
#include <string>
#include <cmath>
using namespace std;


string PasswordInput(string message)
{
    string password;
    
        cout << message;
        cin >> password;
    
    return password;
}

bool checkPassword(string password, string guess)
{
    return password == guess;
}
bool guessPassword(string password)
{
    for(char i ='A';i<='Z';i++) 
    {
        for(char j='A';j<='Z';j++)
        {
            for(char k='A';k<='Z';k++)
            {
                string guess = "";
                guess += i;
                guess += j;
                guess += k;
                cout << "Guessing: " << guess << endl;
                if(checkPassword(password, guess))
                {
                    cout << "Password found: " << guess << endl;
                    return true;
                }
                


            }   
        }

    }
}


void printpattern(string password)
{
    guessPassword(password);
  

    
}
int main()
{

printpattern(PasswordInput("Enter a 3-letter password (A-Z): "));
    

    
 return 0;     
}



