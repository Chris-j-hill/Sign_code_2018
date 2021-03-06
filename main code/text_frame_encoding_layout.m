
% 

                          % parity bits as 8th bit
                          %|/
frame_raw = [0 0 0 0 0 0 1 1;% 1 bit parity check 
             0 0 0 1 1 1 1 1;% hamming (31,26) of all header using 5 remaining bits from row 1 and 2
             0 0 0 1 0 0 0 1;% parity check frame num and num frames            
             0 0 0 0 1 1 1 1;% 3 bit final checksum
             
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 1;
             0 0 0 0 0 0 0 0;% parity check columns
             0 0 0 0 0 0 0 0];%8 bit final checksum

         %%
[h, g, n, k] = hammgen(5);
% g = g(1:16,1:31);
binaryVectorToHex(g);

% for higher redundancy
% [h, g, n, k] = hammgen(8);  %3 bits of redundancy
% g = g(1:23,1:31);

% word = [randi([0,1],1,26)];% zeros(1,10)];

word = [0 0 0 1  0 1 1 0 1 0 0 1 1 0 0 1 1 0 0 1 1 0 1 0 0 1];
code_word = mod(word*g,2);

%%
syndrome = zeros(31,5);
for i = 1:31
    code_word(i) = mod(code_word(i)+1,2);

    syndrome(i,:) = mod(code_word*transpose(h),2);
end

for i = 1:30
    
    for j = i+1:31
        if(syndrome(i,:) == syndrome(j,:))
            disp([num2str(i),'and ',num2str(j)]);
        end
    end
    
end
disp('done');

disp(binaryVectorToHex(h));

syn = zeros(31,2);
syn(:,2) = 1:31;
for i = 1:31
    code_word(i) = mod(code_word(i)+1,2);
    sum=0;
    for j = 1:5
        sum = sum +(mod(h(j,:)*transpose(code_word),2))*(2^(j-1));
    end
    syn(i,1) = sum;
    code_word(i) = mod(code_word(i)+1,2);
end 
syn = sortrows(syn);
disp(syn(:,2));

%%


k=1;
for i = 1:31
    j=i+1;
    code2 = code_word;
    code2(i) = mod(code_word(i)+1,2);
   while j<=31
       code3 = code2;
       code3(j) = mod(code2(j)+1,2);
       syn2(k,:) = mod(code3*transpose(h),2);
       
       j = j+ 1;
       k=k+1;
   end
end




%%
syndrome_dec = zeros(2,length(syndrome(:,1)));
syndrome_dec(1,:) = bi2de(syndrome(:,:));
syndrome_dec(2,:) = 1:31;

syndrome_dec = transpose(sortrows(transpose (syndrome_dec)));


fprintf(['const byte syndrome[] PROGMEM = {']);

syndrome_length = length(syndrome_dec);
 syndrome_dec = [syndrome_dec zeros(2,5)];
syndrome_length2 = syndrome_length;
count=0;
i=1;
while i <=  syndrome_length
    if(syndrome_dec(1,i) ~= i-1+count)  %row does not exist
       for j = syndrome_length2+1+count:-1:i
        syndrome_dec(1,j) = syndrome_dec(1,j-1);
        syndrome_dec(2,j) = syndrome_dec(2,j-1);
       end
       syndrome_dec(1,i) = i-1;
       syndrome_dec(2,i) = 255;
%        count= count+1;
       syndrome_length= syndrome_length+1;
       
    end
    
    fprintf('%s, ',num2str(syndrome_dec(2,i)))
    
    i = i+1;
end
fprintf ('};');
