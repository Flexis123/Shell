void prompt_cat(char* src, size_t *dest_end){
    size_t f = 0;
    for(size_t i = *dest_end + 1; i <= *dest_end + sizeof(src); i++){
        prompt[i] = src[f];
        
        f++;
    }
    *dest_end += sizeof(src);
}