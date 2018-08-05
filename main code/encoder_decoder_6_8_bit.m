clc

word = [0 1 1 0 0 1];

Ik = eye(6);
P = [0 1 1;
     1 0 0;    
     1 0 1;
     1 1 0;
     1 0 0;
     0 1 1];

 
G = [P Ik];


for i = 1:8
    codeword = mod(word*G,2);
    codeword(i) = mod(codeword(i)+1,2);
    H = [eye(3) transpose(P)];
    syndrome = mod(codeword*transpose(H),2)
end


 