int cssmain(){
    const char* example_path = "examples/sample.css";
    size_t content_size;
    char* content_data = (char*)read_entire_file(example_path, &content_size);
    char* content = content_data;
    remove_carrige_return(content);

    CSSNodes cssNodes = {0};
    int result = parse_css_file(content,&cssNodes);
    if(result < 0) {
        printf("CSSERR: %s\n", csserr_str(result));
        return 1;
    }

    printf("Parsed: %zu\n\n", cssNodes.len);
    for(size_t i = 0; i < cssNodes.len; i++){
        CSSNode* node = &cssNodes.items[i];
        printf("-------------------\n");
        printf("Name: %.*s\n", (int)node->name_len, node->name_content);
        if(node->attrs.len > 0) printf("attrs:\n");
        for(size_t j = 0; j < node->attrs.len; j++){
            CSSAttr* attr = &node->attrs.items[j];
            printf("name: %.*s, value: %.*s\n", (int)attr->name_len, attr->name_content,(int)attr->value_len,attr->value_content);
        }
        printf("-------------------\n");
    }

    return 0;
}