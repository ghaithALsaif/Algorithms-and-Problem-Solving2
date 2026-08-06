#include <iostream>
#include <string>
#include <cmath>
using namespace std;

short EncryptionKey = 3;

string TextInput(string message)
{
    string text;
    
        cout << message;
        getline(cin, text);
    
    return text;
}

string EncryptText(string text)
{
    string encryptedText = "";

    for(char c =0 ; c < text.length(); c++)
    {
        encryptedText +=text[c]+EncryptionKey;
    }
    
    return encryptedText;
}

string DecryptText(string text)
{
    string decryptedText = "";

    for(char c =0 ; c < text.length(); c++)
    {
        decryptedText +=text[c]-EncryptionKey;
    }
    
    return decryptedText;
}
void DisplayText(string text)
{
    string EncryptedText = EncryptText(text);
    cout << "\nEncrypted Text: \n" << EncryptedText << endl;
    cout << "\nDecrypted Text: \n" << DecryptText(EncryptedText) << endl;
}

int main()
{

DisplayText(TextInput("Enter text to encrypt: "));

    
 return 0;
}



