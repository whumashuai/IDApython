def printtofile(variablename,cur_addr):
        file2 = open("E:\\IDAADress.txt","w")
        file2.write( "%s :"% variablename +'\n')
        file2.write("0x%x"% cur_addr +'\n')
        file2.close()
        return
def FindVariableName(str):
        variablename = str[6:]
        cur_addr=MinEA()
        end=MaxEA()
        while cur_addr <= end:
                str1 = idc.GetOpnd(cur_addr,0)
                str2 = idc.GetOpnd(cur_addr,1)
                if((variablename in str1)or(variablename in str2)):
                        print "%s :"% variablename
                        print "0x%x"% cur_addr
                        printtofile(variablename,cur_addr)
                        break
                else:
                        cur_addr = idc.NextAddr(cur_addr)
       
        return 




file_object = open("E:\\test10.txt","r")
try:
        symtag ='SymTagData'
        name='Name:'
        lines = file_object.readlines()
        index =0
        for line in lines:
                if(symtag in line):
                        index = 1
                if((name in line)and (index ==1)):
                        index =0
                        line=line.strip('\n')
                        FindVariableName(line)
                        
finally:
        file_object.close()

